#include "Shaders.h"

#include "S3.h"
#include <stdio.h>
#include <stdlib.h>
#include "Utils.h"

#pragma warning(disable : 4244)		//conversion from double to float


#define VERSION_STRING		"#version 460\n"


void _set_uniform_matrix(GLuint program_id, const char* name, Mat4& mat)
{
	float temp[16];
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			temp[j * 4 + i] = mat.data[i][j];
	glProgramUniformMatrix4fv(program_id, glGetUniformLocation(program_id, name), 1, false, temp);
}


Shader::Shader(ShaderCore* core, const std::set<const char*> options) : core(core), options(options)
{
	id = glCreateShader(core->shader_type);

	printf("new shader id = %d\n", id);
	fprintf(stderr, "%s\n", core->name);
	for(auto option : options)
		fprintf(stderr, "\t%s", option);

	for(auto option : options)
		if(!core->options.count(option))
			error("ShaderCore %s doesn't have option %s", core->name, option);

	std::vector<const char*> text;
	text.push_back(VERSION_STRING);
	for(auto option: options)
		text.push_back(option);
	text.push_back(core->core_text);

	glShaderSource(id, text.size(), &text[0], NULL);
	glCompileShader(id);

	GLint success = 0;
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);
	if(success == GL_FALSE)
	{
		for(const char* temp : text)
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
	
	init_func = [core, options](GLuint program_id) {
		if(core->init_func)
			core->init_func(program_id);
		for(auto option : options)
		{
			ShaderPullFunc temp = core->options[option]->init_func;
			if(temp)
				temp(program_id);
		}
	};

	frame_func = [core, options](GLuint program_id) {
		if(core->frame_func)
			core->frame_func(program_id);
		for(auto option : options)
		{
			ShaderPullFunc temp = core->options[option]->frame_func;
			if(temp)
				temp(program_id);
		}
	};

	all_shaders.push_back(this);
}

Shader* Shader::get(ShaderCore* core, std::set<const char*> options)
{
	for(auto shader : all_shaders)
		if(shader->core == core && shader->options == options)
			return shader;
	return new Shader(core, options);
}

std::vector<Shader*> Shader::all_shaders;


ShaderProgram::ShaderProgram(Shader* vert, Shader* geom, Shader* frag)
{
	vertex = vert;
	geometry = geom;
	fragment = frag;

	id = glCreateProgram();
	printf("new program id = %d (%d, %d, %d)\n", id, vertex->get_id(), geometry->get_id(), fragment->get_id());
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


ShaderCore *vert, *geom_points, *geom_triangles, *frag;

void init_shaders()
{
	vert = new ShaderCore(
		"vert",
		GL_VERTEX_SHADER,
		R"(
			layout (location = 0) in vec4 position;

			#ifdef VERTEX_COLOR
				layout (location = 1) in vec4 color;
				out vec4 vg_color;
			#endif

			#ifdef INSTANCED_XFORM
				layout (location = 2) in mat4 modelXForm;
				uniform mat4 viewXForm;
			#else
				uniform mat4 modelViewXForm;
			#endif
			
			#ifdef INSTANCED_BASE_COLOR
				layout (location = 6) in vec4 base_color;
				out vec4 vg_base_color;
			#endif

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
				#ifdef INSTANCED_XFORM
					mat4 modelViewXForm = transpose(viewXForm) * modelXForm;
				#endif
				vg_r4pos = modelViewXForm * position;
				float distance = acos(vg_r4pos.w);
				gl_Position.xyz = distance * normalize(vg_r4pos.xyz);
				gl_Position.w = 1;

				#ifdef INSTANCED_BASE_COLOR
					vg_base_color = base_color;
				#endif
				#ifdef VERTEX_COLOR
					vg_color = color;
				#endif
			}
		)",
		NULL,
		NULL,
		std::vector<ShaderOption*> {
			new ShaderOption(DEFINE_VERTEX_COLOR, NULL, NULL),
			new ShaderOption(
				DEFINE_INSTANCED_XFORM,
				NULL,
				[](int program_id) {
					_set_uniform_matrix(program_id, "viewXForm", cam_mat);
				}
			),
			new ShaderOption(DEFINE_INSTANCED_BASE_COLOR, NULL, NULL)
		}
	);

	geom_points = new ShaderCore(
		"geom_points",
		GL_GEOMETRY_SHADER,
		R"(
			layout (points, invocations = 2) in;
			layout (triangle_strip, max_vertices = 16) out;

			uniform mat4 projXForm;
			uniform float aspectRatio;

			#ifdef VERTEX_COLOR
				in vec4 vg_color[];
				out vec4 gf_color;
			#endif

			#ifdef INSTANCED_BASE_COLOR
				in vec4 vg_base_color[];
				out vec4 gf_base_color;
			#endif

			in vec4 vg_r4pos[];
			out vec4 gf_r4pos;

			out float distance;

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

				gf_r4pos = vg_r4pos[0];
				#ifdef VERTEX_COLOR
					gf_color = vg_color[0];
				#endif
				#ifdef INSTANCED_BASE_COLOR
					gf_base_color = vg_base_color[0];
				#endif
					
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
		)",
		[](int program_id) {
			glProgramUniform1f(program_id, glGetUniformLocation(program_id, "aspectRatio"), aspect_ratio);
			_set_uniform_matrix(program_id, "projXForm", proj_mat);
		},
		NULL,
		std::vector<ShaderOption*> {
			new ShaderOption(DEFINE_VERTEX_COLOR, NULL, NULL),
			new ShaderOption(DEFINE_INSTANCED_BASE_COLOR, NULL, NULL)
		}
	);

	geom_triangles = new ShaderCore(
		"geom_triangles",
		GL_GEOMETRY_SHADER,
		R"(
			layout (triangles, invocations = 2) in;
			layout (triangle_strip, max_vertices = 3) out;

			uniform mat4 projXForm;

			#ifdef VERTEX_COLOR
				in vec4 vg_color[];
				out vec4 gf_color;
			#endif

			#ifdef INSTANCED_BASE_COLOR
				in vec4 vg_base_color[];
				out vec4 gf_base_color;
			#endif

			in vec4 vg_r4pos[];
			out vec4 gf_r4pos;
			out float distance;

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
					#ifdef VERTEX_COLOR
						gf_color = vg_color[i];
					#endif
					#ifdef INSTANCED_BASE_COLOR
						gf_base_color = vg_base_color[i];
					#endif
					gf_r4pos = vg_r4pos[i];
					EmitVertex();
				}
				EndPrimitive();
			}
		)",
		[](int program_id) {
			glProgramUniform1f(program_id, glGetUniformLocation(program_id, "aspectRatio"), aspect_ratio);
			_set_uniform_matrix(program_id, "projXForm", proj_mat);
		},
		NULL,
		std::vector<ShaderOption*> {
			new ShaderOption(DEFINE_VERTEX_COLOR, NULL, NULL),
			new ShaderOption(DEFINE_INSTANCED_BASE_COLOR, NULL, NULL)
		}
	);

	frag = new ShaderCore(
		"frag",
		GL_FRAGMENT_SHADER,
		R"(
			uniform float fogScale;

			#ifdef VERTEX_COLOR
				in vec4 gf_color;
			#endif

			#ifdef INSTANCED_BASE_COLOR
				in vec4 gf_base_color;
			#else
				uniform vec4 baseColor;
			#endif
			
			in vec4 gf_r4pos;
			in float distance;
				
			out vec4 fragColor;
			layout (depth_any) out float gl_FragDepth;


			void main() {
				float dist = clamp(distance / (length(gf_r4pos) * 6.283185), 0, 1);
				float fogFactor = exp(-dist * fogScale);
				#ifdef INSTANCED_BASE_COLOR
					vec4 baseColor = gf_base_color;
				#endif
				#ifdef VERTEX_COLOR
					fragColor = clamp(baseColor + gf_color, 0, 1) * fogFactor;
				#else
					fragColor = baseColor * fogFactor;
				#endif
				gl_FragDepth = dist;
			}
		)",
		NULL,
		[](GLuint program_id) {
			glProgramUniform1f(program_id, glGetUniformLocation(program_id, "fogScale"), fog_scale);
		},
		std::vector<ShaderOption*> {
			new ShaderOption(DEFINE_VERTEX_COLOR, NULL, NULL),
			new ShaderOption(DEFINE_INSTANCED_BASE_COLOR, NULL, NULL)
		}
	);
}
