#pragma once

#include "GL/glew.h"


extern GLuint gbuffer, gbuffer_albedo, gbuffer_position, gbuffer_normal, gbuffer_depth;
extern GLuint abuffer, abuffer_color;

extern GLuint lbuffer, light_map;

extern bool is_shadow_pass;

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
