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
		for(char* temp : text)
			printf(temp);
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


GLuint vert, geom_points, geom_triangles, frag_simple, frag_vcolor;


void init_shaders()
{
	vert = make_new_shader(
		GL_VERTEX_SHADER,
		std::vector<char*> {
			R"(
				#version 460

				layout (location = 0) in vec4 position;
				layout (location = 1) in vec4 color;

				uniform mat4 modelViewXForm;

				out vec4 vg_color;
				
				void main() {
					vec4 r4_pos = modelViewXForm * position;
					float distance = acos(r4_pos.w);
					gl_Position.xyz = distance * normalize(r4_pos.xyz);
					gl_Position.w = 1;
					vg_color = color;
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

				in vec4 vg_color[];

				out float distance;
				out vec4 gf_color;

				#define BASE_POINT_SIZE		(0.002)

				void main() {
					float card = BASE_POINT_SIZE, diag = 0.7071 * BASE_POINT_SIZE, mult = 1.0 / aspectRatio;

					vec4 point = gl_in[0].gl_Position;

					float dist = length(point.xyz);
					point.xyz *= (dist - gl_InvocationID * 6.283185) / dist;
					distance = abs(dist - gl_InvocationID * 6.283185);

					point = projXForm * point;

					point.xyz /= point.w;
					point.w = 1;

					float size = 1 / sin(dist);

					gf_color = vg_color[0];
					
					gl_Position = point + size * vec4(-card * mult, 0, 0, 0);
					EmitVertex();
					gl_Position = point + vec4(0, 0, 0, 0);
					EmitVertex();
					gl_Position = point + size * vec4(-diag * mult, diag, 0, 0);
					EmitVertex();
					gl_Position = point + size * vec4(0, card, 0, 0);
					EmitVertex();
					EndPrimitive();
					
					gl_Position = point + size * vec4(0, -card, 0, 0);
					EmitVertex();
					gl_Position = point + vec4(0, 0, 0, 0);
					EmitVertex();
					gl_Position = point + size * vec4(-diag * mult, -diag, 0, 0);
					EmitVertex();
					gl_Position = point + size * vec4(-card * mult, 0, 0, 0);
					EmitVertex();
					EndPrimitive();
					
					gl_Position = point + size * vec4(card * mult, 0, 0, 0);
					EmitVertex();
					gl_Position = point + vec4(0, 0, 0, 0);
					EmitVertex();
					gl_Position = point + size * vec4(diag * mult, -diag, 0, 0);
					EmitVertex();
					gl_Position = point + size * vec4(0, -card, 0, 0);
					EmitVertex();
					EndPrimitive();
					
					gl_Position = point + size * vec4(0, card, 0, 0);
					EmitVertex();
					gl_Position = point + vec4(0, 0, 0, 0);
					EmitVertex();
					gl_Position = point + size * vec4(diag * mult, diag, 0, 0);
					EmitVertex();
					gl_Position = point + size * vec4(card * mult, 0, 0, 0);
					EmitVertex();
					EndPrimitive();
				}
			)"

			/*R"(
				#version 460

				layout (points, invocations = 2) in;
				layout (points, max_vertices = 1) out;

				uniform mat4 projXForm;

				in vec4 vg_color[];

				out float distance;
				out vec4 gf_color;

				#define BASE_POINT_SIZE		(4)		//Need a uniform for this, based on screen size.

				void main() {
					vec4 point = gl_in[0].gl_Position;

					float dist = length(point.xyz);
					point.xyz *= (dist - gl_InvocationID * 6.283185) / dist;
					distance = abs(dist - gl_InvocationID * 6.283185);

					point = projXForm * point;

					gl_Position = point;
					gl_PointSize = 3 / sin(dist);
					gf_color = vg_color[0];

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

				uniform mat4 projXForm;

				in vec4 vg_color[];

				out float distance;
				out vec4 gf_color;

				void main() {
					for(int i = 0; i < 3; i++)
					{
						vec4 point = gl_in[i].gl_Position;

						float dist = length(point.xyz);
						point.xyz *= (dist - gl_InvocationID * 6.283185) / dist;
						distance = abs(dist - gl_InvocationID * 6.283185);

						point = projXForm * point;

						gl_Position = point;
						gf_color = vg_color[i];
						EmitVertex();
					}
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
				uniform float fogScale;

				in float distance;
				out vec4 fragColor;

				void main() {
					float fogFactor = exp(-distance * fogScale / 6.283185);
					fragColor = baseColor * fogFactor;
				}
			)"
		}
	);

	frag_vcolor = make_new_shader(
		GL_FRAGMENT_SHADER,
		std::vector<char*> {
			R"(
				#version 460

				uniform vec4 baseColor;
				uniform float fogScale;

				in float distance;
				in vec4 gf_color;

				out vec4 fragColor;

				void main() {
					float fogFactor = exp(-distance * fogScale / 6.283185);
					fragColor = (baseColor + gf_color) * fogFactor;
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
