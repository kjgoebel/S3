#pragma once

#include "GL/glew.h"
#include "Vector.h"
#include <vector>


GLuint make_new_shader(GLuint shader_type, const std::vector<char*> text);
GLuint make_new_program(GLuint vert, GLuint geom, GLuint frag);

extern GLuint	vert,
				
				geom_points,
				geom_triangles,
				
				frag_simple;

void init_shaders();

void set_uniform_matrix(GLuint shader_program, const char* name, Mat4& mat);
