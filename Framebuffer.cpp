#include "Framebuffer.h"

#include "Vector.h"
#include "Utils.h"

#define CHORD2_LUT_SIZE		(64)
#define VIEW_W_LUT_SIZE		(64)


GLuint gbuffer = 0, gbuffer_albedo = 0, gbuffer_position = 0, gbuffer_normal = 0, gbuffer_depth = 0;
GLuint abuffer = 0, abuffer_color = 0;
GLuint lbuffer = 0, light_map = 0;
bool is_shadow_pass = false;

LookupTable* chord2_lut;
LookupTable* view_w_lut;

GLuint fsq_vertex_array = 0;


void init_luts()
{
	float* chord2_data = new float[2 * CHORD2_LUT_SIZE];
	for(int i = 0; i < CHORD2_LUT_SIZE; i++)
	{
		float c = 4.0 * i / (CHORD2_LUT_SIZE - 1);
		chord2_data[2 * i] = 2.0 * asin(0.5 * sqrt(c));
		float temp = sin(chord2_data[2 * i]);
		chord2_data[2 * i + 1] = 1.0 / (temp * temp);
	}
	//1 / sin^2(0) is infinite and my GPU doesn't seem to like infinity.
	//FLT_MAX is also too big, because it makes for a visible discontinuity at distance = 2 / (CHORD2_LUT_SIZE - 1).
	//Could try to model the actual size of point lights.
	chord2_data[1] = 3 * chord2_data[3];
		
	chord2_lut = new LookupTable(GL_TEXTURE_1D, CHORD2_LUT_SIZE, GL_RG32F, GL_RG, chord2_data, 0, 4);
	check_gl_errors("chord squared LUT setup");

	delete[] chord2_data;
}

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
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		glBindTexture(GL_TEXTURE_2D, abuffer_color);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);

		if(light_map_size && light_map)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, light_map);
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
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
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

		/*
			Attach the G-buffer's depth buffer to the abuffer so we can draw things 
			into the scene with depth checking during the accumulation phase.
		*/
		glNamedFramebufferTexture(abuffer, GL_DEPTH_ATTACHMENT, gbuffer_depth, 0);

		if(light_map_size)
		{
			glGenFramebuffers(1, &lbuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, lbuffer);

			glDrawBuffers(0, NULL);		//Lights only need depth.

			glGenTextures(1, &light_map);
			glBindTexture(GL_TEXTURE_CUBE_MAP, light_map);
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
			glNamedFramebufferTexture(lbuffer, GL_DEPTH_ATTACHMENT, light_map, 0);
		}
	}

	if(glCheckNamedFramebufferStatus(gbuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		error("G-Buffer status is %d\n", glCheckNamedFramebufferStatus(gbuffer, GL_FRAMEBUFFER));

	if(glCheckNamedFramebufferStatus(abuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		error("A-Buffer status is %d\n", glCheckNamedFramebufferStatus(abuffer, GL_FRAMEBUFFER));

	if(glCheckNamedFramebufferStatus(lbuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		error("L-Buffer status is %d\n", glCheckNamedFramebufferStatus(lbuffer, GL_FRAMEBUFFER));

}

void use_gbuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(true);
	glEnable(GL_CULL_FACE);
}

void use_lbuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, lbuffer);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(true);
	glEnable(GL_CULL_FACE);
}

void use_abuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, abuffer);

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
