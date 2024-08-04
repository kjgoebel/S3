#pragma once

#include "GL/glew.h"
#include "Utils.h"

#pragma warning(disable : 4244)		//conversion from double to float


extern GLuint s_gbuffer, s_gbuffer_albedo, s_gbuffer_position, s_gbuffer_normal, s_gbuffer_depth;
extern GLuint s_abuffer, s_abuffer_color;

extern bool s_is_shadow_pass;

extern int window_width, window_height;


/*
	Call this in reshape(). It can be called multiple times.
	If light_map_size is zero the first time init_framebuffer() is called, lbuffer and light_map will not be available.
*/
void init_framebuffer(int w, int h);

void use_gbuffer();
void use_abuffer();
void use_default_framebuffer();

void draw_fsq();

//half-screen quad, left and right
void draw_hsq(int i);

//quarter-screen quad, ul, ur, ll, lr
void draw_qsq(int i);
