#include "Framebuffer.h"

#include "Vector.h"
#include "Utils.h"


GLuint s_gbuffer = 0, s_gbuffer_albedo = 0, s_gbuffer_position = 0, s_gbuffer_normal = 0, s_gbuffer_depth = 0;
GLuint s_abuffer = 0, s_abuffer_color = 0;
GLuint s_lbuffer = 0, s_light_map = 0;
bool s_is_shadow_pass = false;

int window_width = 0, window_height = 0;

GLuint fsq_vertex_array = 0;


void init_framebuffer(int w, int h, int light_map_size)
{
	glClearColor(0, 0, 0, 0);
	//glEnable(GL_PROGRAM_POINT_SIZE);
	glCullFace(GL_BACK);

	if(!fsq_vertex_array)
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

	if(s_gbuffer)		//init_framebuffer() has been called before
	{
		glBindFramebuffer(GL_FRAMEBUFFER, s_gbuffer);
		glBindTexture(GL_TEXTURE_2D, s_gbuffer_albedo);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, s_gbuffer_position);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, s_gbuffer_normal);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, s_gbuffer_depth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		glBindTexture(GL_TEXTURE_2D, s_abuffer_color);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);

		if(light_map_size && s_light_map)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, s_light_map);
			for(int face = 0; face < 6; face++)
				glTexImage2D(
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
					0,
					GL_DEPTH_COMPONENT32F,
					light_map_size,
					light_map_size,
					0,
					GL_DEPTH_COMPONENT,
					GL_FLOAT,
					NULL
				);
		}
	}
	else
	{
		glGenFramebuffers(1, &s_gbuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, s_gbuffer);

		glGenTextures(1, &s_gbuffer_albedo);
		glBindTexture(GL_TEXTURE_2D, s_gbuffer_albedo);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glNamedFramebufferTexture(s_gbuffer, GL_COLOR_ATTACHMENT0, s_gbuffer_albedo, 0);

		glGenTextures(1, &s_gbuffer_position);
		glBindTexture(GL_TEXTURE_2D, s_gbuffer_position);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glNamedFramebufferTexture(s_gbuffer, GL_COLOR_ATTACHMENT1, s_gbuffer_position, 0);

		glGenTextures(1, &s_gbuffer_normal);
		glBindTexture(GL_TEXTURE_2D, s_gbuffer_normal);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glNamedFramebufferTexture(s_gbuffer, GL_COLOR_ATTACHMENT2, s_gbuffer_normal, 0);
		
		glGenTextures(1, &s_gbuffer_depth);
		glBindTexture(GL_TEXTURE_2D, s_gbuffer_depth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glNamedFramebufferTexture(s_gbuffer, GL_DEPTH_ATTACHMENT, s_gbuffer_depth, 0);

		GLuint attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
		glDrawBuffers(3, attachments);

		glGenFramebuffers(1, &s_abuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, s_abuffer);

		glGenTextures(1, &s_abuffer_color);
		glBindTexture(GL_TEXTURE_2D, s_abuffer_color);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glNamedFramebufferTexture(s_abuffer, GL_COLOR_ATTACHMENT0, s_abuffer_color, 0);

		/*
			Attach the G-buffer's depth buffer to the abuffer so we can draw things 
			into the scene with depth checking during the accumulation phase.
		*/
		glNamedFramebufferTexture(s_abuffer, GL_DEPTH_ATTACHMENT, s_gbuffer_depth, 0);

		if(light_map_size)
		{
			glGenFramebuffers(1, &s_lbuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, s_lbuffer);

			glDrawBuffers(0, NULL);		//Lights only need depth.

			glGenTextures(1, &s_light_map);
			glBindTexture(GL_TEXTURE_CUBE_MAP, s_light_map);
			for(int face = 0; face < 6; face++)
				glTexImage2D(
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
					0,
					GL_DEPTH_COMPONENT32F,
					light_map_size,
					light_map_size,
					0,
					GL_DEPTH_COMPONENT,
					GL_FLOAT,
					NULL
				);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			glNamedFramebufferTexture(s_lbuffer, GL_DEPTH_ATTACHMENT, s_light_map, 0);
		}
	}

	if(glCheckNamedFramebufferStatus(s_gbuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		error("G-Buffer status is %d\n", glCheckNamedFramebufferStatus(s_gbuffer, GL_FRAMEBUFFER));

	if(glCheckNamedFramebufferStatus(s_abuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		error("A-Buffer status is %d\n", glCheckNamedFramebufferStatus(s_abuffer, GL_FRAMEBUFFER));

	if(glCheckNamedFramebufferStatus(s_lbuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		error("L-Buffer status is %d\n", glCheckNamedFramebufferStatus(s_lbuffer, GL_FRAMEBUFFER));

}

void use_gbuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, s_gbuffer);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(true);
	glEnable(GL_CULL_FACE);
}

void use_lbuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, s_lbuffer);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(true);
	glEnable(GL_CULL_FACE);
}

void use_abuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, s_abuffer);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
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
