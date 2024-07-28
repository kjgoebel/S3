#pragma once

#include "GL/glew.h"


extern GLuint gbuffer, gbuffer_albedo, gbuffer_position, gbuffer_normal, gbuffer_depth;

void init_framebuffer(int w, int h);		//Call this in reshape(). It can be called multiple times.
void use_gbuffer();
void use_default_framebuffer();

void draw_fsq();
