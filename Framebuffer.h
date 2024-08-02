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

	Using a uniform also allows us to automate the squeezing of the range: if 
	your input is supposed to go from 1 to 4, pass in a domain_offset of 1 and 
	a domain_size of 3. Those will be applied to the generated offset and scale 
	so that your shader doesn't have to do separate work for that. Your shader 
	will have
		texture(blah_lut, argument * blah_lut_scale + blah_lut_offset)
*/

private:
	GLenum target;
	GLuint texture;
	float offset, scale;

public:
	//Assume for now that all LUT textures will be square (so that offset and scale can be scalars).
	LookupTable(GLenum targ, GLsizei tex_size, GLenum internal_format, GLenum format, const float* data, float domain_offset, float domain_size)
	{
		target = targ;
		offset = 0.5 / tex_size - domain_offset / domain_size;
		scale = ((float)tex_size - 1) / (tex_size * domain_size);

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
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);		//This doesn't do anything, but it makes dumps of the texture look better.
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

/*
	LUT for chord length
	c2 = (chord length)^2 ->
		R) 2 asin(1/2 sqrt(c2))
			(the S3 distance for that chord)
		G) 1 / sin^2(2 asin(1/2 sqrt(c2)))
			(the intensity factor for a point light at that distance)
*/
extern LookupTable* chord2_lut;

/*
	LUT for "view" space (R4 with camera at (0, 0, 0, 1)) W coordinate
	pos.w ->
		R) acos(pos.w)
			(the S3 distance from the camera to pos)
		G) acos(pos.w) / sqrt(1 - (pos.w)^2)
			(which is the normalization factor for pos.xyz times the S3 distance)
*/
extern LookupTable* view_w_lut;

void init_luts();

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
