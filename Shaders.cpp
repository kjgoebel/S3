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


GLuint vert, geom_points, geom_triangles, frag_simple;


void init_shaders()
{
	vert = make_new_shader(
		GL_VERTEX_SHADER,
		std::vector<char*> {
			R"(
				#version 460

				layout (location = 0) in vec4 position;

				uniform mat4 fullTransform;
				
				void main() {
					gl_Position = fullTransform * position;
					gl_Position.w = 1;
					//gl_Position = vec4(position.x + float(fullTransform[0][1]), position.y, position.z, 1);
				}
			)"
		}
	);

	geom_points = make_new_shader(
		GL_GEOMETRY_SHADER,
		std::vector<char*> {
			R"(
				#version 460

				layout (points, invocations = 2) in;
				layout (triangle_strip, max_vertices = 16) out;

				void main() {
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.05, -0.05, 0, 0) + vec4(0, 0.01, 0, 0);
					EmitVertex();
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.05, -0.05, 0, 0) + vec4(0, 0, 0, 0);
					EmitVertex();
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.05, -0.05, 0, 0) + vec4(-0.007071, 0.007071, 0, 0);
					EmitVertex();
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.05, -0.05, 0, 0) + vec4(-0.01, 0, 0, 0);
					EmitVertex();
					EndPrimitive();

					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.05, -0.05, 0, 0) + vec4(-0.01, 0, 0, 0);
					EmitVertex();
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.05, -0.05, 0, 0) + vec4(0, 0, 0, 0);
					EmitVertex();
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.05, -0.05, 0, 0) + vec4(-0.007071, -0.007071, 0, 0);
					EmitVertex();
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.05, -0.05, 0, 0) + vec4(0, -0.01, 0, 0);
					EmitVertex();
					EndPrimitive();

					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.05, -0.05, 0, 0) + vec4(0, -0.01, 0, 0);
					EmitVertex();
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.05, -0.05, 0, 0) + vec4(0, 0, 0, 0);
					EmitVertex();
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.05, -0.05, 0, 0) + vec4(0.007071, -0.007071, 0, 0);
					EmitVertex();
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.05, -0.05, 0, 0) + vec4(0.01, 0, 0, 0);
					EmitVertex();
					EndPrimitive();

					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.05, -0.05, 0, 0) + vec4(0.01, 0, 0, 0);
					EmitVertex();
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.05, -0.05, 0, 0) + vec4(0, 0, 0, 0);
					EmitVertex();
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.05, -0.05, 0, 0) + vec4(0.007071, 0.007071, 0, 0);
					EmitVertex();
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.05, -0.05, 0, 0) + vec4(0, 0.01, 0, 0);
					EmitVertex();
					EndPrimitive();
				}
			)"

			/*R"(
				#version 460

				layout (points, invocations = 2) in;
				layout (triangle_strip, max_vertices = 4) out;

				void main() {
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.25, -0.25, 0, 0) + vec4(0.01, 0.01, 0, 0);
					EmitVertex();
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.25, -0.25, 0, 0) + vec4(-0.01, 0.01, 0, 0);
					EmitVertex();
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.25, -0.25, 0, 0) + vec4(0.01, -0.01, 0, 0);
					EmitVertex();
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(-0.25, -0.25, 0, 0) + vec4(-0.01, -0.01, 0, 0);
					EmitVertex();
					EndPrimitive();
				}
			)"*/
		}
	);
	
	geom_triangles = make_new_shader(
		GL_GEOMETRY_SHADER,
		std::vector<char*> {
			R"(
				#version 460

				layout (triangles, invocations = 2) in;
				layout (triangle_strip, max_vertices = 3) out;

				void main() {
					gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(0.25, -0.5, 0, 0);
					EmitVertex();
					gl_Position = gl_in[1].gl_Position + gl_InvocationID * vec4(0.25, -0.5, 0, 0);
					EmitVertex();
					gl_Position = gl_in[2].gl_Position + gl_InvocationID * vec4(0.25, -0.5, 0, 0);
					EmitVertex();
					EndPrimitive();
				}
			)"
		}
	);
	
	frag_simple = make_new_shader(
		GL_FRAGMENT_SHADER,
		std::vector<char*> {
			R"(
				#version 460

				uniform vec4 baseColor;
				out vec4 fragColor;

				void main() {
					fragColor = baseColor;
				}
			)"
		}
	);
}


void set_uniform_matrix(GLuint shader_program, const char* name, Mat4& mat)
{
	float temp[16];
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			temp[j * 4 + i] = mat.data[i][j];
	glProgramUniformMatrix4fv(shader_program, glGetUniformLocation(shader_program, name), 1, false, temp);
}
