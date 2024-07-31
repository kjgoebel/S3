#pragma once

#include "GL/glew.h"
#include "Utils.h"

#pragma warning(disable : 4244)		//conversion from double to float


class LookupTable
{
/*
	The problem with lookup tables is that texture coordinates go from zero to one, 
	but actual texels go from zero to (texture_size - 1). The first and last 
	0.5 / texture_size of texture coordinates are useless (unless you want a LUT 
	for a function for which a border color would be useful). So we calculate 
	offset = 0.5 / texture_size and scale = (texture_size - 1) / texture_size. 
	offset and scale will be fed to the shader.
	
	Using textureSize() within the shader is another possibility, but it's slower. 
	Even if textureSize() is fast, calculating offset and scale involve int to 
	float conversion and more arithmetic than "scale * thing + offset".
*/

private:
	GLenum target;
	GLuint texture;
	float offset, scale;

public:
	//Assume for now that all LUT textures will be square (so that offset and scale can be scalars).
	LookupTable(GLenum targ, GLsizei tex_size, GLenum internal_format, GLenum format, const float* data)
	{
		target = targ;
		offset = 0.5 / tex_size;
		scale = ((float)tex_size - 1) / tex_size;

		glGenTextures(1, &texture);
		glBindTexture(target, texture);

		switch(target)
		{
			case GL_TEXTURE_1D:
				glTexImage1D(target, 0, internal_format, tex_size, 0, format, GL_FLOAT, data);
				break;
			case GL_TEXTURE_2D:
				glTexImage2D(target, 0, internal_format, tex_size, tex_size, 0, format, GL_FLOAT, data);
				break;
			default:
				error("LUTs for target %d are not implemented.\n", target);
				break;
		}
		
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	GLenum get_target() {return target;}
	GLuint get_texture() {return texture;}
	float get_offset() {return offset;}
	float get_scale() {return scale;}
};


extern GLuint gbuffer, gbuffer_albedo, gbuffer_position, gbuffer_normal, gbuffer_depth;
extern GLuint abuffer, abuffer_color;

extern GLuint lbuffer, light_map;

extern bool is_shadow_pass;

extern LookupTable* chord_distance_lut;

/*
	Call this in reshape(). It can be called multiple times.
	If light_map_size is zero the first time init_framebuffer() is called, lbuffer and light_map will not be available.
*/
void init_framebuffer(int w, int h, int light_map_size = 0);

void use_gbuffer();
void use_lbuffer();
void use_abuffer();
void use_default_framebuffer();

void draw_fsq();

//half-screen quad, left and right
void draw_hsq(int i);

//quarter-screen quad, ul, ur, ll, lr
void draw_qsq(int i);
