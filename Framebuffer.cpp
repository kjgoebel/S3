#include "Framebuffer.h"

#include "Vector.h"
#include "Utils.h"


GBuffer* s_gbuffer = NULL;
ABuffer* s_abuffer = NULL;

int window_width = 0, window_height = 0;

GLuint fsq_vertex_array = 0;


TextureSpec::TextureSpec(GLenum target, GLenum internal_format, GLenum format, GLenum type, GLenum attachment_point, GLenum min_filter, GLenum mag_filter, GLenum wrap_mode)
{
	this->target = target;
	this->internal_format = internal_format;
	this->format = format;
	this->type = type;
	this->attachment_point = attachment_point;
	this->min_filter = min_filter;
	this->mag_filter = mag_filter;
	this->wrap_mode = wrap_mode;
}

void TextureSpec::make_texture(GLuint* out_tex_name, GLuint framebuffer, GLsizei width, GLsizei height)
{
	check_gl_errors("TextureSpec::make_texture() 0");

	glGenTextures(1, out_tex_name);
	tex_image(*out_tex_name, width, height, NULL);
	
	check_gl_errors("TextureSpec::make_texture() 1");

	glBindTexture(target, *out_tex_name);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, mag_filter);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap_mode);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap_mode);
	glTexParameteri(target, GL_TEXTURE_WRAP_R, wrap_mode);
	if(format == GL_DEPTH_COMPONENT)
		glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);

	check_gl_errors("TextureSpec::make_texture() 2");

	//framebuffer has to be bound as GL_FRAMEBUFFER, or glNamedFramebufferTexture() doesn't work (?!)
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glNamedFramebufferTexture(framebuffer, attachment_point, *out_tex_name, 0);

	check_gl_errors("TextureSpec::make_texture() 3");
}

void TextureSpec::tex_image(GLuint tex_name, GLsizei width, GLsizei height, void* data)
{
	glBindTexture(target, tex_name);
	switch(target)
	{
		case GL_TEXTURE_1D:
			glTexImage1D(target, 0, internal_format, width, 0, format, type, data);
			break;
		case GL_TEXTURE_2D:
			glTexImage2D(target, 0, internal_format, width, height, 0, format, type, data);
			break;
		case GL_TEXTURE_CUBE_MAP:
			for(int face = 0; face < 6; face++)
				glTexImage2D(
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
					0,
					internal_format,
					width,
					height,
					0,
					format,
					type,
					data
				);
			break;
		default:
			error("Framebuffer textures of type %d are not implemented.", target);
			break;
	}
}


Framebuffer::Framebuffer()
{
	glGenFramebuffers(1, &name);
}


void Screenbuffer::post_resize_all()
{
	for(auto sb : all_screenbuffers)
		sb->post_resize();
}

std::vector<Screenbuffer*> Screenbuffer::all_screenbuffers;


TextureSpec GBuffer::albedo_spec(GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT0),
	GBuffer::position_spec(GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT1),
	GBuffer::normal_spec(GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT2),
	GBuffer::depth_spec(GL_TEXTURE_2D, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_ATTACHMENT);

GBuffer::GBuffer() : Screenbuffer()
{
	width = window_width;
	height = window_height;

	glBindFramebuffer(GL_FRAMEBUFFER, name);

	albedo_spec.make_texture(&albedo, name, window_width, window_height);
	position_spec.make_texture(&position, name, window_width, window_height);
	normal_spec.make_texture(&normal, name, window_width, window_height);
	depth_spec.make_texture(&depth, name, window_width, window_height);

	GLuint attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
	glDrawBuffers(3, attachments);

	/*
		Wait till post_resize() to check the status, because framebuffers are 
		created before the first call to resize(), so they start with textures 
		of zero size.
	*/

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GBuffer::post_resize()
{
	Screenbuffer::post_resize();

	albedo_spec.tex_image(albedo, window_width, window_height);
	position_spec.tex_image(position, window_width, window_height);
	normal_spec.tex_image(normal, window_width, window_height);
	depth_spec.tex_image(depth, window_width, window_height);

	/*
		So, glCheckNamedFramebufferStatus() returns incomplete attachment unless 
		the named framebuffer is currently bound as GL_FRAMEBUFFER. In other words, 
		glCheckNamedFramebufferStatus() doesn't work, I guess.
	*/
	glBindFramebuffer(GL_FRAMEBUFFER, name);
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		error("G-Buffer %d status is 0x%X\n", name, glCheckFramebufferStatus(GL_FRAMEBUFFER));
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


TextureSpec ABuffer::color_spec(GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT0);

ABuffer::ABuffer(GBuffer* gbuffer) : Screenbuffer()
{
	width = window_width;
	height = window_height;

	glBindFramebuffer(GL_FRAMEBUFFER, name);

	color_spec.make_texture(&color, name, window_width, window_height);
	
	/*
		Attach the G-buffer's depth buffer so we can draw things into the 
		scene with depth checking during the accumulation phase.
	*/
	glNamedFramebufferTexture(name, GL_DEPTH_ATTACHMENT, gbuffer->depth, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ABuffer::post_resize()
{
	Screenbuffer::post_resize();

	color_spec.tex_image(color, window_width, window_height);
	
	glBindFramebuffer(GL_FRAMEBUFFER, name);
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		error("G-Buffer %d status is 0x%X\n", name, glCheckFramebufferStatus(GL_FRAMEBUFFER));
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


const Pass* Pass::current = NULL;

Pass::Pass(Framebuffer* fb)
{
	framebuffer = fb;
	clear_mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
	depth_test = depth_mask = true;
	blend = false;
	cull_face = GL_BACK;
	is_shadow_pass = false;
}

void Pass::start() const
{
	if(framebuffer)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->name);
		glViewport(0, 0, framebuffer->width, framebuffer->height);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, window_width, window_height);
	}
	depth_test ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
	glDepthMask(depth_mask ? GL_TRUE : GL_FALSE);
	if(cull_face)
	{
		glEnable(GL_CULL_FACE);
		glCullFace(cull_face);
	}
	else
		glDisable(GL_CULL_FACE);
	blend ? glEnable(GL_BLEND) : glDisable(GL_BLEND);

	//glClear has to be called at the end because glDepthMask() affects clearing.
	if(clear_mask)
		glClear(clear_mask);
	current = this;
}


void init_screenspace()
{
	Vec4 fsq_vertices[4 * 7] = {
		//full-screen quad
		{1, 1, 0, 1},
		{-1, 1, 0, 1},
		{-1, -1, 0, 1},
		{1, -1, 0, 1},

		//half-screen quads
		{0, 1, 0, 1},
		{-1, 1, 0, 1},
		{-1, -1, 0, 1},
		{0, -1, 0, 1},

		{1, 1, 0, 1},
		{0, 1, 0, 1},
		{0, -1, 0, 1},
		{1, -1, 0, 1},

		//quarter-screen quads
		{0, 1, 0, 1},
		{-1, 1, 0, 1},
		{-1, 0, 0, 1},
		{0, 0, 0, 1},

		{1, 1, 0, 1},
		{0, 1, 0, 1},
		{0, 0, 0, 1},
		{1, 0, 0, 1},

		{0, 0, 0, 1},
		{-1, 0, 0, 1},
		{-1, -1, 0, 1},
		{0, -1, 0, 1},

		{1, 0, 0, 1},
		{0, 0, 0, 1},
		{0, -1, 0, 1},
		{1, -1, 0, 1}
	};

	float fsq_tex_coords[4 * 7][2];
	for(int i = 0; i < 7; i++)
	{
		fsq_tex_coords[4 * i + 0][0] = 1;
		fsq_tex_coords[4 * i + 0][1] = 1;

		fsq_tex_coords[4 * i + 1][0] = 0;
		fsq_tex_coords[4 * i + 1][1] = 1;

		fsq_tex_coords[4 * i + 2][0] = 0;
		fsq_tex_coords[4 * i + 2][1] = 0;

		fsq_tex_coords[4 * i + 3][0] = 1;
		fsq_tex_coords[4 * i + 3][1] = 0;
	}

	glGenVertexArrays(1, &fsq_vertex_array);
	glBindVertexArray(fsq_vertex_array);

	GLuint fsq_vertex_buffer;
	glGenBuffers(1, &fsq_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, fsq_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(fsq_vertices), fsq_vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 4, GL_DOUBLE, GL_FALSE, sizeof(Vec4), (void*)0);
	glEnableVertexAttribArray(0);

	GLuint fsq_tex_coord_buffer;
	glGenBuffers(1, &fsq_tex_coord_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, fsq_tex_coord_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(fsq_tex_coords), fsq_tex_coords, GL_STATIC_DRAW);
		
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
}


void resize_screenbuffers(int w, int h)
{
	window_width = w;
	window_height = h;

	Screenbuffer::post_resize_all();
}


void draw_fsq()
{
	glBindVertexArray(fsq_vertex_array);
	glDrawArrays(GL_QUADS, 0, 4);
}

void draw_hsq(int i)
{
	glBindVertexArray(fsq_vertex_array);
	glDrawArrays(GL_QUADS, 4 + 4 * i, 4);
}

void draw_qsq(int i)
{
	glBindVertexArray(fsq_vertex_array);
	glDrawArrays(GL_QUADS, 12 + 4 * i, 4);
}
