#include "Shaders.h"

#include "S3.h"
#include <stdio.h>
#include <stdlib.h>

#pragma warning(disable : 4244)		//conversion from double to float


void _set_uniform_matrix(GLuint program_id, const char* name, Mat4& mat)
{
	float temp[16];
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			temp[j * 4 + i] = mat.data[i][j];
	glProgramUniformMatrix4fv(program_id, glGetUniformLocation(program_id, name), 1, false, temp);
}


Shader::Shader(GLuint shader_type, const std::vector<char*> text, ShaderPullFunc init_func, ShaderPullFunc frame_func)
{
	id = glCreateShader(shader_type);
	glShaderSource(id, text.size(), &text[0], NULL);
	glCompileShader(id);

	GLint success = 0;
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);
	if(success == GL_FALSE)
	{
		for(char* temp : text)
			printf(temp);
		GLint log_length = 0;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);
		char* log = new char[log_length + 1];
		glGetShaderInfoLog(id, log_length, &log_length, log);
		log[log_length] = 0;
		printf(log);

		delete[] log;
		exit(-1);
	}

	this->init_func = init_func;
	this->frame_func = frame_func;
}


ShaderProgram::ShaderProgram(Shader* vert, Shader* geom, Shader* frag)
{
	vertex = vert;
	geometry = geom;
	fragment = frag;

	id = glCreateProgram();
	glAttachShader(id, vertex->get_id());
	glAttachShader(id, geometry->get_id());
	glAttachShader(id, fragment->get_id());
	glLinkProgram(id);

	GLint success;
	glGetProgramiv(id, GL_LINK_STATUS, &success);
	if(success == GL_FALSE)
	{
		GLint log_length = 0;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_length);
		char* log = new char[log_length + 1];
		glGetProgramInfoLog(id, log_length, &log_length, log);
		log[log_length] = 0;
		printf(log);

		delete[] log;
		exit(-1);
	}

	init();

	all_shader_programs.push_back(this);
}

void ShaderProgram::set_uniform_matrix(const char* name, Mat4& mat)
{
	//Don't like the function call overhead....
	_set_uniform_matrix(id, name, mat);
}

ShaderProgram* ShaderProgram::get(Shader* vert, Shader* geom, Shader* frag)
{
	for(ShaderProgram* temp : all_shader_programs)
		if(temp->vertex == vert && temp->geometry == geom && temp->fragment == frag)
			return temp;
	return new ShaderProgram(vert, geom, frag);
}

void ShaderProgram::init_all()
{
	for(ShaderProgram* temp : all_shader_programs)
		temp->init();
}

void ShaderProgram::frame_all()
{
	for(ShaderProgram* temp : all_shader_programs)
		temp->frame();
}

std::vector<ShaderProgram*> ShaderProgram::all_shader_programs;


Shader *vert, *geom_points, *geom_triangles, *frag_simple, *frag_vcolor;


void init_shaders()
{
	vert = new Shader(
		GL_VERTEX_SHADER,
		std::vector<char*> {
			R"(
				#version 460

				layout (location = 0) in vec4 position;
				layout (location = 1) in vec4 color;

				uniform mat4 modelViewXForm;

				out vec4 vg_color;

				/*
					Pass along the transformed position in R4, so that the fragment shader 
					can compensate for the inaccuracy of the depth calculation for the 
					middle of a polygon close to antipode. Note that this is not the 
					position of the vertex relative to the camera. The camera is at 
					(0, 0, 0, 1) in this coordinate system. The position of the vertex on 
					screen doesn't depend on the w coordinate (once the camera has been 
					transformed to x = y = z = 0), and it's convenient for the fragment 
					shader to have the Sphere centered on the origin.
				*/
				out vec4 vg_r4pos;
				
				void main() {
					vg_r4pos = modelViewXForm * position;
					float distance = acos(vg_r4pos.w);
					gl_Position.xyz = distance * normalize(vg_r4pos.xyz);
					gl_Position.w = 1;
					vg_color = color;
				}
			)"
		}
	);

	geom_points = new Shader(
		GL_GEOMETRY_SHADER,
		std::vector<char*> {
			R"(
				#version 460

				layout (points, invocations = 2) in;
				layout (triangle_strip, max_vertices = 16) out;

				uniform mat4 projXForm;
				uniform float aspectRatio;

				in vec4 vg_color[];
				in vec4 vg_r4pos[];

				out float distance;
				out vec4 gf_color;
				out vec4 gf_r4pos;

				#define BASE_POINT_SIZE		(0.002)

				void main() {
					float card = BASE_POINT_SIZE, diag = 0.7071 * BASE_POINT_SIZE, mult = 1.0 / aspectRatio;

					vec4 point = gl_in[0].gl_Position;

					float dist = length(point.xyz);
					float image_dist = dist - gl_InvocationID * 6.283185;
					point.xyz *= image_dist / dist;
					distance = abs(image_dist);

					point = projXForm * point;

					point.xyz /= point.w;
					point.w = 1;

					float size = 1 / sin(dist);

					gf_color = vg_color[0];
					gf_r4pos = vg_r4pos[0];
					
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
		},
		[](int program_id) {
			glProgramUniform1f(program_id, glGetUniformLocation(program_id, "aspectRatio"), aspect_ratio);
			_set_uniform_matrix(program_id, "projXForm", proj_mat);
		},
		NULL
	);

	geom_triangles = new Shader(
		GL_GEOMETRY_SHADER,
		std::vector<char*> {
			R"(
				#version 460

				layout (triangles, invocations = 2) in;
				layout (triangle_strip, max_vertices = 3) out;

				uniform mat4 projXForm;

				in vec4 vg_color[];
				in vec4 vg_r4pos[];

				out float distance;
				out vec4 gf_color;
				out vec4 gf_r4pos;

				void main() {
					for(int i = 0; i < 3; i++)
					{
						vec4 point = gl_in[i].gl_Position;

						float dist = length(point.xyz);
						float image_dist = dist - gl_InvocationID * 6.283185;
						point.xyz *= image_dist / dist;
						distance = abs(image_dist);

						point = projXForm * point;

						gl_Position = point;
						gf_color = vg_color[i];
						gf_r4pos = vg_r4pos[i];
						EmitVertex();
					}
					EndPrimitive();
				}
			)"
		},
		[](int program_id) {
			glProgramUniform1f(program_id, glGetUniformLocation(program_id, "aspectRatio"), aspect_ratio);
			_set_uniform_matrix(program_id, "projXForm", proj_mat);
		},
		(ShaderPullFunc)NULL
	);
	
	frag_simple = new Shader(
		GL_FRAGMENT_SHADER,
		std::vector<char*> {
			R"(
				#version 460

				uniform vec4 baseColor;
				uniform float fogScale;

				in float distance;
				in vec4 gf_r4pos;

				layout (depth_any) out float gl_FragDepth;
				out vec4 fragColor;

				void main() {
					float dist = clamp(distance / (length(gf_r4pos) * 6.283185), 0, 1);
					float fogFactor = exp(-dist * fogScale);
					fragColor = baseColor * fogFactor;
					gl_FragDepth = dist;
				}
			)"
		},
		NULL,
		[](GLuint program_id) {
			glProgramUniform1f(program_id, glGetUniformLocation(program_id, "fogScale"), fog_scale);
		}
	);

	frag_vcolor = new Shader(
		GL_FRAGMENT_SHADER,
		std::vector<char*> {
			R"(
				#version 460

				uniform vec4 baseColor;
				uniform float fogScale;

				in float distance;
				in vec4 gf_r4pos;
				in vec4 gf_color;
				
				layout (depth_any) out float gl_FragDepth;
				out vec4 fragColor;

				void main() {
					float dist = clamp(distance / (length(gf_r4pos) * 6.283185), 0, 1);
					float fogFactor = exp(-dist * fogScale);
					fragColor = clamp(baseColor + gf_color, 0, 1) * fogFactor;
					gl_FragDepth = dist;
				}
			)"
		},
		NULL,
		[](GLuint program_id) {
			glProgramUniform1f(program_id, glGetUniformLocation(program_id, "fogScale"), fog_scale);
		}
	);
}
