#pragma once

#include "GL/glew.h"
#include "Vector.h"


GLuint make_new_shader(const char** text, int count);
GLuint make_new_program(GLuint vert, GLuint geom, GLuint frag);

extern GLuint	vert,
				
				geom_points,
				geom_triangles,
				
				frag_simple;

void init_shaders();

void set_uniform_matrix(GLuint shader_program, const char* name, Mat4& mat);
