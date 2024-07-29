#include "Shaders.h"

#include "S3.h"
#include <stdio.h>
#include <stdlib.h>
#include "Utils.h"
#include "Framebuffer.h"

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

	use_func = [core, options](GLuint program_id) {
		if(core->use_func)
			core->use_func(program_id);
		for(auto option : options)
		{
			ShaderPullFunc temp = core->options[option]->use_func;
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
	printf("new program id = %d (%d, %d, %d)\n", id, vertex->get_id(), geometry ? geometry->get_id() : -1, fragment->get_id());
	glAttachShader(id, vertex->get_id());
	if(geometry)
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

void ShaderProgram::dump() const
{
	printf("Shader program id %d:\n", id);
	for(auto shader : std::vector<Shader*> {vertex, geometry, fragment})
		if(shader)
		{
			printf("\tShader id %d:\n", shader->get_id());
			printf("\t\t%s\n", shader->get_core()->name);
			for(auto option : shader->get_options())
				printf("\t\t\t%s", option);
		}
		else
			printf("None");
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
ShaderCore *vert_screenspace, *frag_copy_textures, *frag_fog, *frag_point_light, *frag_dump_color;

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

			#ifdef VERTEX_NORMAL
				layout (location = 2) in vec4 normal;
				out vec4 vg_normal;
			#endif

			#ifdef INSTANCED_XFORM
				layout (location = 3) in mat4 modelXForm;
				uniform mat4 viewXForm;
			#else
				uniform mat4 modelViewXForm;
			#endif
			
			#ifdef INSTANCED_BASE_COLOR
				layout (location = 7) in vec4 base_color;
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
				#ifdef VERTEX_NORMAL
					vg_normal = modelViewXForm * normal;
				#endif
			}
		)",
		NULL,
		NULL,
		NULL,
		{
			new ShaderOption(DEFINE_VERTEX_COLOR, NULL, NULL),
			new ShaderOption(DEFINE_VERTEX_NORMAL, NULL, NULL),
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

			#ifdef VERTEX_NORMAL
				in vec4 vg_normal[];
				out vec4 gf_normal;
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
				#ifdef VERTEX_NORMAL
					gf_normal = vg_normal[0];
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
		NULL,
		{
			new ShaderOption(DEFINE_VERTEX_COLOR, NULL, NULL),
			new ShaderOption(DEFINE_VERTEX_NORMAL, NULL, NULL),
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

			#ifdef VERTEX_NORMAL
				in vec4 vg_normal[];
				out vec4 gf_normal;
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
					#ifdef VERTEX_NORMAL
						gf_normal = vg_normal[i];
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
		NULL,
		{
			new ShaderOption(DEFINE_VERTEX_COLOR, NULL, NULL),
			new ShaderOption(DEFINE_VERTEX_NORMAL, NULL, NULL),
			new ShaderOption(DEFINE_INSTANCED_BASE_COLOR, NULL, NULL)
		}
	);

	frag = new ShaderCore(
		"frag",
		GL_FRAGMENT_SHADER,
		R"(
			#ifdef VERTEX_COLOR
				in vec4 gf_color;
			#endif

			#ifdef VERTEX_NORMAL
				in vec4 gf_normal;
			#endif

			#ifdef INSTANCED_BASE_COLOR
				in vec4 gf_base_color;
			#else
				uniform vec4 baseColor;
			#endif
			
			in vec4 gf_r4pos;
			in float distance;
				
			layout (location = 0) out vec4 frag_albedo;
			layout (location = 1) out vec4 frag_position;
			layout (location = 2) out vec4 frag_normal;
			layout (depth_any) out float gl_FragDepth;

			void main() {
				#ifdef INSTANCED_BASE_COLOR
					vec4 baseColor = gf_base_color;
				#endif
				#ifdef VERTEX_COLOR
					frag_albedo = clamp(baseColor + gf_color, 0, 1);
				#else
					frag_albedo = baseColor;
				#endif

				float bulge_factor = length(gf_r4pos);		//I'm pretty sure this is wrong.
				frag_position = gf_r4pos / bulge_factor;

				#ifdef VERTEX_NORMAL
					frag_normal = normalize(gf_normal);
				#else
					frag_normal = vec4(0, 0, 1, 0);
				#endif

				gl_FragDepth = clamp(distance / (bulge_factor * 6.283185), 0, 1);
			}
		)",
		NULL,
		NULL,
		NULL,
		{
			new ShaderOption(DEFINE_VERTEX_COLOR, NULL, NULL),
			new ShaderOption(DEFINE_VERTEX_NORMAL, NULL, NULL),
			new ShaderOption(DEFINE_INSTANCED_BASE_COLOR, NULL, NULL)
		}
	);


	vert_screenspace = new ShaderCore(
		"vert_screenspace",
		GL_VERTEX_SHADER,
		R"(
			layout (location = 0) in vec4 position;

			void main() {
				gl_Position = position;
			}
		)",
		NULL,
		NULL,
		NULL,
		{}
	);

	frag_copy_textures = new ShaderCore(
		"frag_copy_textures",
		GL_FRAGMENT_SHADER,
		R"(
			uniform sampler2D albedo_tex;
			uniform sampler2D position_tex;
			uniform sampler2D normal_tex;
			uniform sampler2D depth_tex;
			
			out vec4 frag_color;

			void main() {
				ivec2 pixel_coords = ivec2(gl_FragCoord.xy);
				float checker_x = float((pixel_coords.x >> 5) & 1), checker_y = float((pixel_coords.y >> 5) & 1);

				if((pixel_coords.x & 0x100) != 0)
					if((pixel_coords.y & 0x100) != 0)
						frag_color = texelFetch(albedo_tex, pixel_coords, 0);
					else
						//This shows the whole range, but sqeezes it so much you can't see anything:
						frag_color = texelFetch(position_tex, pixel_coords, 0) / (2 * 6.283185) + 0.5;
				else
					if((pixel_coords.y & 0x100) != 0)
						frag_color = texelFetch(normal_tex, pixel_coords, 0) * 0.5 + 0.5;
					else
						frag_color = texelFetch(depth_tex, pixel_coords, 0);
			}
		)",
		NULL,
		NULL,
		[](GLuint program_id) {
			glUniform1i(glGetUniformLocation(program_id, "albedo_tex"), 0);
			glActiveTexture(GL_TEXTURE0 + 0);
			glBindTexture(GL_TEXTURE_2D, gbuffer_albedo);

			glUniform1i(glGetUniformLocation(program_id, "position_tex"), 1);
			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, gbuffer_position);
			
			glUniform1i(glGetUniformLocation(program_id, "normal_tex"), 2);
			glActiveTexture(GL_TEXTURE0 + 2);
			glBindTexture(GL_TEXTURE_2D, gbuffer_normal);

			glUniform1i(glGetUniformLocation(program_id, "depth_tex"), 3);
			glActiveTexture(GL_TEXTURE0 + 3);
			glBindTexture(GL_TEXTURE_2D, gbuffer_depth);
		},
		{}
	);

	frag_fog = new ShaderCore(
		"frag_fog",
		GL_FRAGMENT_SHADER,
		R"(
			uniform sampler2D albedo_tex;
			uniform sampler2D depth_tex;

			uniform float fog_scale;
			
			out vec4 frag_color;

			void main() {
				ivec2 pixel_coords = ivec2(gl_FragCoord.xy);
				float distance = texelFetch(depth_tex, pixel_coords, 0).r;
				vec4 color = texelFetch(albedo_tex, pixel_coords, 0);
				frag_color = exp(-distance * fog_scale) * color;
			}
		)",
		NULL,
		[](GLuint program_id) {
			glProgramUniform1f(program_id, glGetUniformLocation(program_id, "fog_scale"), fog_scale);
		},
		[](GLuint program_id) {
			glUniform1i(glGetUniformLocation(program_id, "albedo_tex"), 0);
			glActiveTexture(GL_TEXTURE0 + 0);
			glBindTexture(GL_TEXTURE_2D, gbuffer_albedo);
			glUniform1i(glGetUniformLocation(program_id, "depth_tex"), 1);
			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, gbuffer_depth);
		},
		{}
	);

	frag_point_light = new ShaderCore(
		"frag_point_light",
		GL_FRAGMENT_SHADER,
		R"(
			uniform sampler2D albedo_tex;
			uniform sampler2D position_tex;
			uniform sampler2D normal_tex;

			uniform vec4 light_pos;
			uniform vec3 light_emission;

			out vec4 frag_color;

			void main() {
				ivec2 pixel_coords = ivec2(gl_FragCoord.xy);
				vec4 color = texelFetch(albedo_tex, pixel_coords, 0);
				vec4 position = texelFetch(position_tex, pixel_coords, 0);
				vec4 normal = texelFetch(normal_tex, pixel_coords, 0);

				vec4 frag_to_light = light_pos - position;
				float r4_distance = length(frag_to_light);
				frag_to_light = normalize(frag_to_light - position);
				float intensity_factor = sin(2 * asin(0.5 * r4_distance));
				intensity_factor = 1 / (intensity_factor * intensity_factor);
				float normal_factor = clamp(dot(normal, frag_to_light), 0, 1);

				frag_color.rgb = color.rgb * light_emission * normal_factor * intensity_factor;
				frag_color.a = 1;
			}
		)",
		NULL,
		NULL,
		[](GLuint program_id) {
			glUniform1i(glGetUniformLocation(program_id, "albedo_tex"), 0);
			glActiveTexture(GL_TEXTURE0 + 0);
			glBindTexture(GL_TEXTURE_2D, gbuffer_albedo);
			
			glUniform1i(glGetUniformLocation(program_id, "position_tex"), 1);
			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, gbuffer_position);
			
			glUniform1i(glGetUniformLocation(program_id, "normal_tex"), 2);
			glActiveTexture(GL_TEXTURE0 + 2);
			glBindTexture(GL_TEXTURE_2D, gbuffer_normal);
		},
		{}
	);

	frag_dump_color = new ShaderCore(
		"frag_dump_color",
		GL_FRAGMENT_SHADER,
		R"(
			uniform sampler2D color_tex;

			out vec4 frag_color;

			void main() {
				ivec2 pixel_coords = ivec2(gl_FragCoord.xy);
				vec3 color = texelFetch(color_tex, pixel_coords, 0).rgb;
				float value = max(color.r, max(color.g, color.b));
				frag_color.rgb = tanh(value) * color / value;
				frag_color.a = 1;
			}
		)",
		NULL,
		NULL,
		[](GLuint program_id) {
			glUniform1i(glGetUniformLocation(program_id, "color_tex"), 0);
			glActiveTexture(GL_TEXTURE0 + 0);
			glBindTexture(GL_TEXTURE_2D, abuffer_color);
		},
		{}
	);
}
