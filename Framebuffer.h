#pragma once

#include "GL/glew.h"


extern GLuint gbuffer, gbuffer_albedo, gbuffer_position, gbuffer_normal, gbuffer_depth;
extern GLuint abuffer, abuffer_color;

extern bool is_shadow_pass;

void init_framebuffer(int w, int h);		//Call this in reshape(). It can be called multiple times.
void use_gbuffer();
void use_abuffer();
void use_default_framebuffer();

void draw_fsq();
