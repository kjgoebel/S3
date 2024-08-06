#include "Framebuffer.h"

#include "Vector.h"
#include "Utils.h"


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

GLuint TextureSpec::make_texture(GLuint framebuffer, GLsizei width, GLsizei height)
{
	check_gl_errors("TextureSpec::make_texture() 0");

	GLuint ret;
	glGenTextures(1, &ret);
	tex_image(ret, width, height, NULL);
	
	check_gl_errors("TextureSpec::make_texture() 1");

	glBindTexture(target, ret);
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
	glNamedFramebufferTexture(framebuffer, attachment_point, ret, 0);

	check_gl_errors("TextureSpec::make_texture() 3");

	return ret;
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


Framebuffer::Framebuffer(const char* display_name, std::vector<TextureSpec> texture_specs, std::vector<ExtraAttachment> extra_attachments, GLsizei width, GLsizei height)
{
	this->width = width;
	this->height = height;

	this->display_name = display_name;

	this->texture_specs = texture_specs;

	check_gl_errors("Framebuffer::Framebuffer() 0");

	glGenFramebuffers(1, &name);
	glBindFramebuffer(GL_FRAMEBUFFER, name);

	check_gl_errors("Framebuffer::Framebuffer() 1");

	std::vector<GLenum> attachment_points;

	for(auto& spec : texture_specs)
	{
		textures.push_back(spec.make_texture(name, width, height));
		if(spec.attachment_point >= GL_COLOR_ATTACHMENT0 && spec.attachment_point <= GL_COLOR_ATTACHMENT15)
			attachment_points.push_back(spec.attachment_point);
	}

	check_gl_errors("Framebuffer::Framebuffer() 2");

	glDrawBuffers(attachment_points.size(), attachment_points.data());

	for(auto extra : extra_attachments)
		glNamedFramebufferTexture(name, extra.attachment_point, extra.tex_name, 0);

	check_gl_errors("Framebuffer::Framebuffer() 3");

	/*
		We don't check the status here because Screenbuffers are created 
		with zero-size textures. Wait until they're resized.
	*/

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::check_status() const
{
	/*
		So, glCheckNamedFramebufferStatus() returns incomplete attachment unless 
		the named framebuffer is currently bound as GL_FRAMEBUFFER. In other words, 
		glCheckNamedFramebufferStatus() doesn't work, I guess.
	*/
	glBindFramebuffer(GL_FRAMEBUFFER, name);
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		error("%s %d status is 0x%X\n", display_name, name, glCheckFramebufferStatus(GL_FRAMEBUFFER));
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


Screenbuffer::Screenbuffer(const char* display_name, std::vector<TextureSpec> texture_specs, std::vector<ExtraAttachment> extra_attachments, GLsizei width, GLsizei height)
	: Framebuffer(display_name, texture_specs, extra_attachments, width, height)
{
	all_screenbuffers.push_back(this);
}

void Screenbuffer::post_resize()
{
	width = window_width;
	height = window_height;

	for(int i = 0; i < texture_specs.size(); i++)
		texture_specs[i].tex_image(textures[i], window_width, window_height);
}

void Screenbuffer::post_resize_all()
{
	for(auto sb : all_screenbuffers)
		sb->post_resize();
}

std::vector<Screenbuffer*> Screenbuffer::all_screenbuffers;


Screenbuffer* s_gbuffer;


const Pass* Pass::current = NULL;

Pass::Pass(Framebuffer* fb, Pass* copy)
{
	framebuffer = fb;

	if(copy)
	{
		clear_mask = copy->clear_mask;
		depth_test = copy->depth_test;
		depth_mask = copy->depth_mask;
		blend = copy->blend;
		cull_face = copy->cull_face;
		is_shadow_pass = copy->is_shadow_pass;
	}
	else
	{
		clear_mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
		depth_test = depth_mask = true;
		blend = false;
		cull_face = GL_BACK;
		is_shadow_pass = false;
	}
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


void init_framebuffers()
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

	glBindVertexArray(0);

	s_gbuffer = new Screenbuffer(
		"G-Buffer",
		{
			{GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT0},
			{GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT1},
			{GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT2},
			{GL_TEXTURE_2D, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_ATTACHMENT},
			{GL_TEXTURE_2D, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, GL_COLOR_ATTACHMENT3}
		},
		{}
	);
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
	glBindVertexArray(0);
}

void draw_hsq(int i)
{
	glBindVertexArray(fsq_vertex_array);
	glDrawArrays(GL_QUADS, 4 + 4 * i, 4);
	glBindVertexArray(0);
}

void draw_qsq(int i)
{
	glBindVertexArray(fsq_vertex_array);
	glDrawArrays(GL_QUADS, 12 + 4 * i, 4);
	glBindVertexArray(0);
}
