#include "Shaders.h"

#include <stdio.h>
#include <stdlib.h>


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

				uniform mat4 modelViewXForm;
				
				void main() {
					vec4 r4_pos = modelViewXForm * position;
					float lr4 = length(r4_pos);
					float ls3 = 2 * asin(0.5 * lr4);
					gl_Position.xyz = ls3 / lr4 * r4_pos.xyz;
					gl_Position.w = 1;
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

				uniform mat4 projXForm;
				uniform float aspectRatio;

				#define BASE_POINT_SIZE		(0.002)

				void main() {
					float card = BASE_POINT_SIZE, diag = 0.7071 * BASE_POINT_SIZE, mult = 1.0 / aspectRatio;

					vec4 point = gl_in[0].gl_Position;

					float distance = length(point.xyz);
					point.xyz *= (distance - gl_InvocationID * 6.283185) / distance;

					point = projXForm * point;

					point.xyz /= point.w;
					point.w = 1;

					float size = 1 / sin(distance);

					gl_Position = point + size * vec4(0, card, 0, 0);
					EmitVertex();
					gl_Position = point + vec4(0, 0, 0, 0);
					EmitVertex();
					gl_Position = point + size * vec4(-diag * mult, diag, 0, 0);
					EmitVertex();
					gl_Position = point + size * vec4(-card * mult, 0, 0, 0);
					EmitVertex();
					EndPrimitive();

					gl_Position = point + size * vec4(-card * mult, 0, 0, 0);
					EmitVertex();
					gl_Position = point + vec4(0, 0, 0, 0);
					EmitVertex();
					gl_Position = point + size * vec4(-diag * mult, -diag, 0, 0);
					EmitVertex();
					gl_Position = point + size * vec4(0, -card, 0, 0);
					EmitVertex();
					EndPrimitive();

					gl_Position = point + size * vec4(0, -card, 0, 0);
					EmitVertex();
					gl_Position = point + vec4(0, 0, 0, 0);
					EmitVertex();
					gl_Position = point + size * vec4(diag * mult, -diag, 0, 0);
					EmitVertex();
					gl_Position = point + size * vec4(card * mult, 0, 0, 0);
					EmitVertex();
					EndPrimitive();

					gl_Position = point + size * vec4(card * mult, 0, 0, 0);
					EmitVertex();
					gl_Position = point + vec4(0, 0, 0, 0);
					EmitVertex();
					gl_Position = point + size * vec4(diag * mult, diag, 0, 0);
					EmitVertex();
					gl_Position = point + size * vec4(0, card, 0, 0);
					EmitVertex();
					EndPrimitive();
				}
			)"

			/*R"(
				#version 460

				layout (points, invocations = 2) in;
				layout (points, max_vertices = 1) out;

				uniform mat4 projXForm;

				void main() {
					vec4 point = gl_in[0].gl_Position;

					float distance = length(point.xyz);
					point.xyz *= (distance - gl_InvocationID * 3.141593) / distance;

					point = projXForm * point;

					gl_Position = point;
					gl_PointSize = 3 / sin(distance);

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
