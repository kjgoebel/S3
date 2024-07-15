#include "Shaders.h"

#include <stdio.h>
#include <stdlib.h>
#include <vector>


GLuint make_new_shader(GLuint shader_type, const std::vector<char*> text)
{
	GLuint ret = glCreateShader(shader_type);
	glShaderSource(ret, text.size(), &text[0], NULL);
	glCompileShader(ret);

	GLint success = 0;
	glGetShaderiv(ret, GL_COMPILE_STATUS, &success);
	if(success == GL_FALSE)
	{
		GLint log_length = 0;
		glGetShaderiv(ret, GL_INFO_LOG_LENGTH, &log_length);
		char* log = new char[log_length + 1];
		glGetShaderInfoLog(ret, log_length, &log_length, log);
		log[log_length] = 0;
		printf(log);

		delete log;
		exit(-1);
	}

	return ret;
}


GLuint make_new_program(GLuint vert, GLuint geom, GLuint frag)
{
	GLuint ret = glCreateProgram();
	glAttachShader(ret, vert);
	glAttachShader(ret, geom);
	glAttachShader(ret, frag);
	glLinkProgram(ret);

	GLint success;
	glGetProgramiv(ret, GL_LINK_STATUS, &success);
	if(success == GL_FALSE)
	{
		GLint log_length = 0;
		glGetProgramiv(ret, GL_INFO_LOG_LENGTH, &log_length);
		char* log = new char[log_length + 1];
		glGetProgramInfoLog(ret, log_length, &log_length, log);
		log[log_length] = 0;
		printf(log);

		delete log;
		exit(-1);
	}

	return ret;
}


GLuint vert, geom_triangles, frag_simple;


void init_shaders()
{
	vert = make_new_shader(
		GL_VERTEX_SHADER,
		std::vector<char*> {
			"#version 460\n"
			"layout (location = 0) in vec3 position;\n"
			"void main() {\
				gl_Position = vec4(position.x, position.y, position.z, 1);\
			}\n"
		}
	);
	
	geom_triangles = make_new_shader(
		GL_GEOMETRY_SHADER,
		std::vector<char*> {
			"#version 460\n"
			"layout (invocations = 2) in;\n"
			"layout (triangles) in;\n"
			"layout (triangle_strip, max_vertices = 3) out;\n"
			"void main() {\n"
				"gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(0.25, -0.5, 0, 0);\n"
				"EmitVertex();\n"
				"gl_Position = gl_in[1].gl_Position + gl_InvocationID * vec4(0.25, -0.5, 0, 0);\n"
				"EmitVertex();\n"
				"gl_Position = gl_in[2].gl_Position + gl_InvocationID * vec4(0.25, -0.5, 0, 0);\n"
				"EmitVertex();\n"
				"EndPrimitive();\n"
			"}\n"
		}
	);
	
	frag_simple = make_new_shader(
		GL_FRAGMENT_SHADER,
		std::vector<char*> {
			"#version 460\n"
			"out vec4 fragColor;\n"
			"void main() {\
				fragColor = vec4(0.5, 0, 0.5, 1);\
			}\n"
		}
	);
}
