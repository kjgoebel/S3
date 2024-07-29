#include "Framebuffer.h"

#include "Vector.h"
#include "Utils.h"


GLuint gbuffer = 0, gbuffer_albedo = 0, gbuffer_position = 0, gbuffer_normal = 0, gbuffer_depth = 0;
GLuint abuffer, abuffer_color;
bool is_shadow_pass = false;
GLuint fsq_vertex_array = 0;


void init_framebuffer(int w, int h)
{
	glClearColor(0, 0, 0, 0);
	//glEnable(GL_PROGRAM_POINT_SIZE);
	glCullFace(GL_BACK);

	if(!fsq_vertex_array)
	{
		Vec4 fsq_vertices[4] = {
			{1, 1, 0, 1},
			{-1, 1, 0, 1},
			{-1, -1, 0, 1},
			{1, -1, 0, 1}
		};

		glGenVertexArrays(1, &fsq_vertex_array);
		glBindVertexArray(fsq_vertex_array);

		GLuint fsq_vertex_buffer;
		glGenBuffers(1, &fsq_vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, fsq_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(fsq_vertices), fsq_vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 4, GL_DOUBLE, GL_FALSE, sizeof(Vec4), (void*)0);
		glEnableVertexAttribArray(0);
	}

	if(gbuffer)		//init_framebuffer() has been called before
	{
		glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);
		glBindTexture(GL_TEXTURE_2D, gbuffer_albedo);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, gbuffer_position);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, gbuffer_normal);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, gbuffer_depth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		glBindFramebuffer(GL_FRAMEBUFFER, abuffer);
		glBindTexture(GL_TEXTURE_2D, abuffer_color);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
	}
	else
	{
		glGenFramebuffers(1, &gbuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);

		glGenTextures(1, &gbuffer_albedo);
		glBindTexture(GL_TEXTURE_2D, gbuffer_albedo);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glNamedFramebufferTexture(gbuffer, GL_COLOR_ATTACHMENT0, gbuffer_albedo, 0);

		glGenTextures(1, &gbuffer_position);
		glBindTexture(GL_TEXTURE_2D, gbuffer_position);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glNamedFramebufferTexture(gbuffer, GL_COLOR_ATTACHMENT1, gbuffer_position, 0);

		glGenTextures(1, &gbuffer_normal);
		glBindTexture(GL_TEXTURE_2D, gbuffer_normal);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glNamedFramebufferTexture(gbuffer, GL_COLOR_ATTACHMENT2, gbuffer_normal, 0);
		
		glGenTextures(1, &gbuffer_depth);
		glBindTexture(GL_TEXTURE_2D, gbuffer_depth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glNamedFramebufferTexture(gbuffer, GL_DEPTH_ATTACHMENT, gbuffer_depth, 0);

		GLuint attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
		glDrawBuffers(3, attachments);

		glGenFramebuffers(1, &abuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, abuffer);

		glGenTextures(1, &abuffer_color);
		glBindTexture(GL_TEXTURE_2D, abuffer_color);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glNamedFramebufferTexture(abuffer, GL_COLOR_ATTACHMENT0, abuffer_color, 0);
	}

	if(glCheckNamedFramebufferStatus(gbuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		error("G-Buffer status is %d\n", glCheckNamedFramebufferStatus(gbuffer, GL_FRAMEBUFFER));

	if(glCheckNamedFramebufferStatus(abuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		error("A-Buffer status is %d\n", glCheckNamedFramebufferStatus(abuffer, GL_FRAMEBUFFER));

}

void use_gbuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

void use_abuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, abuffer);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}

void use_default_framebuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}


void draw_fsq()
{
	glBindVertexArray(fsq_vertex_array);
	glDrawArrays(GL_QUADS, 0, 4);
}
