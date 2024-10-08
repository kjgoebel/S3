#include "Shaders.h"

#include "Camera.h"
#include <stdio.h>
#include <stdlib.h>
#include "Utils.h"
#include "Framebuffer.h"
#include "Light.h"

#pragma warning(disable : 4244)		//conversion from double to float
#pragma warning(disable : 4996)		//VS doesn't like old-school string manipulation.


#define VERSION_STRING		"#version 460\n"


Shader::Shader(ShaderCore* core, const std::set<const char*> options) : core(core), options(options)
{
	id = glCreateShader(core->shader_type);

	fprintf(stderr, "new shader id = %d\n", id);
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
	
	init_func = [core, options](ShaderProgram* program) {
		if(core->init_func)
			core->init_func(program);
		for(auto option : options)
		{
			ShaderPullFunc temp = core->options[option]->init_func;
			if(temp)
				temp(program);
		}
	};

	frame_func = [core, options](ShaderProgram* program) {
		if(core->frame_func)
			core->frame_func(program);
		for(auto option : options)
		{
			ShaderPullFunc temp = core->options[option]->frame_func;
			if(temp)
				temp(program);
		}
	};

	use_func = [core, options](ShaderProgram* program) {
		if(core->use_func)
			core->use_func(program);
		for(auto option : options)
		{
			ShaderPullFunc temp = core->options[option]->use_func;
			if(temp)
				temp(program);
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
	fprintf(stderr, "new program id = %d (%d, %d, %d)\n", id, vertex->get_id(), geometry ? geometry->get_id() : -1, fragment->get_id());
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

void ShaderProgram::set_matrix(const char* name, const Mat4& mat)
{
	float temp[16];
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			temp[j * 4 + i] = mat.data[i][j];
	glProgramUniformMatrix4fv(id, glGetUniformLocation(id, name), 1, false, temp);
}

void ShaderProgram::set_matrices(const char* name, const Mat4* mats, int count)
{
	float* temp = new float[16 * count];
	for(int mat = 0; mat < count; mat++)
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
				temp[mat * 16 + j * 4 + i] = mats[mat].data[i][j];
	glProgramUniformMatrix4fv(id, glGetUniformLocation(id, name), count, false, temp);
	delete[] temp;
}

void ShaderProgram::set_vector(const char* name, const Vec4& v)
{
	glProgramUniform4f(id, glGetUniformLocation(id, name), v.x, v.y, v.z, v.w);
}

void ShaderProgram::set_vector(const char* name, const Vec3& v)
{
	glProgramUniform3f(id, glGetUniformLocation(id, name), v.x, v.y, v.z);
}

void ShaderProgram::set_float(const char* name, float f)
{
	glProgramUniform1f(id, glGetUniformLocation(id, name), f);
}

void ShaderProgram::set_int(const char* name, int i)
{
	glProgramUniform1i(id, glGetUniformLocation(id, name), i);
}

void ShaderProgram::set_texture(const char* name, int tex_unit, GLuint texture, GLenum target)
{
	set_int(name, tex_unit);
	glActiveTexture(GL_TEXTURE0 + tex_unit);
	glBindTexture(target, texture);
}

void ShaderProgram::set_lut(const char* name, int tex_unit, LookupTable* lut)
{
	check_gl_errors("set_lut 0");
	size_t name_length = strlen(name);
	char* temp = new char[name_length + 8];		//8 = strlen("_offset") + 1 for the null char. strlen("_offset") > strlen("_scale").
	strcpy(temp, name);
	
	set_texture(name, tex_unit, lut->get_texture(), lut->get_target());
	strcat(temp, "_scale");

	set_float(temp, lut->get_scale());
	temp[name_length] = 0;
	strcat(temp, "_offset");

	set_float(temp, lut->get_offset());
	
	delete[] temp;
	check_gl_errors("set_lut 1");
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


ShaderCore *vert, *geom_points, *geom_triangles, *frag_points, *frag;
ShaderCore *vert_screenspace;

void init_shaders()
{
	//S3 Shaders
	vert = new ShaderCore(
		"vert",
		GL_VERTEX_SHADER,
		R"(
			layout (location = 0) in vec4 position;

			#ifndef SHADOW
				#ifdef VERTEX_COLOR
					layout (location = 1) in vec4 color;
					out vec4 vg_color;
				#endif
				#ifdef VERTEX_NORMAL
					layout (location = 2) in vec4 normal;
					out vec4 vg_normal;
				#endif
				#ifdef INSTANCED_BASE_COLOR
					layout (location = 7) in vec4 base_color;
					out vec4 vg_base_color;
				#endif
			#endif

			#ifdef INSTANCED_XFORM
				layout (location = 3) in mat4 model_xform;
				uniform mat4 view_xform;
			#else
				uniform mat4 model_view_xform;
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
					mat4 model_view_xform = transpose(view_xform) * model_xform;
				#endif
				vg_r4pos = model_view_xform * position;

				//The next two lines can be replaced with a table lookup, but it makes the shader slower (?!)
				float distance = acos(vg_r4pos.w);
				gl_Position.xyz = distance * normalize(vg_r4pos.xyz);
				gl_Position.w = 1;

				#ifndef SHADOW
					#ifdef INSTANCED_BASE_COLOR
						vg_base_color = base_color;
					#endif
					#ifdef VERTEX_COLOR
						vg_color = color;
					#endif
					#ifdef VERTEX_NORMAL
						vg_normal = model_view_xform * normal;
					#endif
				#endif
			}
		)",
		NULL,
		NULL,
		NULL,
		{
			new ShaderOption(DEFINE_VERTEX_COLOR),
			new ShaderOption(DEFINE_VERTEX_NORMAL),
			new ShaderOption(
				DEFINE_INSTANCED_XFORM,
				NULL,
				NULL,
				[](ShaderProgram* program) {
					program->set_matrix("view_xform", s_curcam->get_mat());
				}
			),
			new ShaderOption(DEFINE_INSTANCED_BASE_COLOR),
			new ShaderOption(DEFINE_SHADOW)
		}
	);

	geom_points = new ShaderCore(
		"geom_points",
		GL_GEOMETRY_SHADER,
		R"(
			layout (points, invocations = 2) in;
			#ifdef SHADOW
				layout (triangle_strip, max_vertices = 24) out;
			#else
				layout (triangle_strip, max_vertices = 4) out;
			#endif

			uniform mat4 proj_xform;
			uniform float aspect_ratio;

			#ifdef SHADOW
				uniform mat4 cube_xforms[6];
			#else
				#ifdef VERTEX_COLOR
					in vec4 vg_color[];
					out vec4 gf_color;
				#endif
				#ifdef INSTANCED_BASE_COLOR
					in vec4 vg_base_color[];
					out vec4 gf_base_color;
				#endif
			#endif

			in vec4 vg_r4pos[];
			out vec4 gf_r4pos;

			out float distance;

			out vec2 point_coord;

			#define BASE_POINT_SIZE		(0.002)

			void main() {
				gf_r4pos = vg_r4pos[0];

				float dist = length(gl_in[0].gl_Position.xyz);
				float distance_factor = 1 / sin(dist);

				#ifndef SHADOW
					#ifdef VERTEX_COLOR
						gf_color = vg_color[0];
					#endif
					#ifdef INSTANCED_BASE_COLOR
						gf_base_color = vg_base_color[0];
					#endif
				#endif
					
				float height = BASE_POINT_SIZE * distance_factor, width = height / aspect_ratio;

				#ifdef SHADOW
					for(int face = 0; face < 6; face++)
					{
						gl_Layer = face;
				#endif
						vec4 point = gl_in[0].gl_Position;

						float image_dist = dist - gl_InvocationID * 6.283185;
						point.xyz *= image_dist / dist;
						distance = abs(image_dist);

						#ifdef SHADOW
							point = proj_xform * cube_xforms[face] * point;
						#else
							point = proj_xform * point;
						#endif

						point.xyz /= point.w;
						point.w = 1;

						gl_Position = point + vec4(-width, -height, 0, 0);
						point_coord = vec2(-1, 1);
						EmitVertex();
						gl_Position = point + vec4(width, -height, 0, 0);
						point_coord = vec2(1, 1);
						EmitVertex();
						gl_Position = point + vec4(-width, height, 0, 0);
						point_coord = vec2(-1, -1);
						EmitVertex();
						gl_Position = point + vec4(width, height, 0, 0);
						point_coord = vec2(1, -1);
						EmitVertex();
						EndPrimitive();
				#ifdef SHADOW
					}
				#endif
			}
		)",
		NULL,
		NULL,
		[](ShaderProgram* program) {
			program->set_float("aspect_ratio", s_curcam->get_aspect_ratio());
			program->set_matrix("proj_xform", s_curcam->get_proj());
		},
		{
			new ShaderOption(DEFINE_VERTEX_COLOR),
			new ShaderOption(DEFINE_INSTANCED_BASE_COLOR),
			new ShaderOption(
				DEFINE_SHADOW,
				[](ShaderProgram* program) {
					program->set_matrices("cube_xforms", s_cube_xforms, 6);
				}
			)
		}
	);

	geom_triangles = new ShaderCore(
		"geom_triangles",
		GL_GEOMETRY_SHADER,
		R"(
			layout (triangles, invocations = 2) in;
			#ifdef SHADOW
				layout (triangle_strip, max_vertices = 18) out;
			#else
				layout (triangle_strip, max_vertices = 3) out;
			#endif

			uniform mat4 proj_xform;

			#ifdef SHADOW
				uniform mat4 cube_xforms[6];
			#else
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
			#endif

			in vec4 vg_r4pos[];
			out vec4 gf_r4pos;

			out float distance;

			void main() {
				#ifdef SHADOW
					for(int face = 0; face < 6; face++)
					{
						gl_Layer = face;
				#endif
						for(int i = 0; i < 3; i++)
						{
							vec4 point = gl_in[i].gl_Position;

							float dist = length(point.xyz);
							float image_dist = dist - gl_InvocationID * 6.283185;
							point.xyz *= image_dist / dist;
							distance = abs(image_dist);

							#ifdef SHADOW
								gl_Position = proj_xform * cube_xforms[face] * point;
							#else
								gl_Position = proj_xform * point;
							#endif
							gf_r4pos = vg_r4pos[i];

							#ifndef SHADOW
								#ifdef VERTEX_COLOR
									gf_color = vg_color[i];
								#endif
								#ifdef VERTEX_NORMAL
									gf_normal = vg_normal[i];
								#endif
								#ifdef INSTANCED_BASE_COLOR
									gf_base_color = vg_base_color[i];
								#endif
							#endif
							EmitVertex();
						}
						EndPrimitive();
				#ifdef SHADOW
					}
				#endif
			}
		)",
		NULL,
		NULL,
		[](ShaderProgram* program) {
			program->set_float("aspect_ratio", s_curcam->get_aspect_ratio());
			program->set_matrix("proj_xform", s_curcam->get_proj());
		},
		{
			new ShaderOption(DEFINE_VERTEX_COLOR),
			new ShaderOption(DEFINE_VERTEX_NORMAL),
			new ShaderOption(DEFINE_INSTANCED_BASE_COLOR),
			new ShaderOption(
				DEFINE_SHADOW,
				[](ShaderProgram* program) {
					program->set_matrices("cube_xforms", s_cube_xforms, 6);
				}
			)
		}
	);

	frag_points = new ShaderCore(
		"frag_points",
		GL_FRAGMENT_SHADER,
		R"(
			#ifndef SHADOW
				#ifdef VERTEX_COLOR
					in vec4 gf_color;
				#endif
				#ifdef INSTANCED_BASE_COLOR
					in vec4 gf_base_color;
				#else
					uniform vec4 base_color;
				#endif
			#endif
			
			in vec4 gf_r4pos;
			in float distance;
			in vec2 point_coord;
				
			#ifndef SHADOW
				layout (location = 0) out vec4 frag_albedo;
				layout (location = 1) out vec4 frag_position;
				layout (location = 2) out vec4 frag_normal;
			#endif
			layout (depth_any) out float gl_FragDepth;

			//This should not be repeated here and in geom_points. It should be a uniform.
			#define BASE_POINT_SIZE		(0.002)

			void main() {
				/*
					This might be a good candidate for a "G-buffer sprite". Eliminate the square root 
					and the discard -> one texture lookup for normal (implies depth) and alpha.
				*/

				float point_coord_l2 = point_coord.x * point_coord.x + point_coord.y * point_coord.y;
				if(point_coord_l2 > 1)
					discard;

				float normal_z = -sqrt(1 - point_coord_l2);

				#ifndef SHADOW
					#ifdef INSTANCED_BASE_COLOR
						vec4 base_color = gf_base_color;
					#endif
					#ifdef VERTEX_COLOR
						frag_albedo = clamp(base_color + gf_color, 0, 1);
					#else
						frag_albedo = base_color;
					#endif

					frag_normal = vec4(point_coord.x, point_coord.y, normal_z, 0);

					frag_position = gf_r4pos + BASE_POINT_SIZE * frag_normal;
				#endif
				
				gl_FragDepth = clamp((distance + BASE_POINT_SIZE * normal_z) / 6.283185, 0, 1);
			}
		)",
		NULL,
		NULL,
		NULL,
		{
			new ShaderOption(DEFINE_VERTEX_COLOR),
			new ShaderOption(DEFINE_INSTANCED_BASE_COLOR),
			new ShaderOption(DEFINE_SHADOW)
		}
	);

	frag = new ShaderCore(
		"frag",
		GL_FRAGMENT_SHADER,
		R"(
			uniform sampler1D chord2_lut;
			uniform float chord2_lut_scale;
			uniform float chord2_lut_offset;

			#ifndef SHADOW
				#ifdef VERTEX_COLOR
					in vec4 gf_color;
				#endif
				#ifdef VERTEX_NORMAL
					in vec4 gf_normal;
				#endif
				#ifdef INSTANCED_BASE_COLOR
					in vec4 gf_base_color;
				#else
					uniform vec4 base_color;
				#endif
			#endif
			
			in vec4 gf_r4pos;
			in float distance;
				
			#ifndef SHADOW
				layout (location = 0) out vec4 frag_albedo;
				layout (location = 1) out vec4 frag_position;
				layout (location = 2) out vec4 frag_normal;
			#endif
			layout (depth_any) out float gl_FragDepth;

			void main() {
				vec4 true_position = normalize(gf_r4pos);

				#ifndef SHADOW
					#ifdef INSTANCED_BASE_COLOR
						vec4 base_color = gf_base_color;
					#endif
					#ifdef VERTEX_COLOR
						frag_albedo = clamp(base_color + gf_color, 0, 1);
					#else
						frag_albedo = base_color;
					#endif
					
					#ifdef VERTEX_NORMAL
						frag_normal = normalize(gf_normal);
					#else
						frag_normal = vec4(0, 0, 0, 0);
					#endif

					frag_position = true_position;
				#endif
				
				/*
					This can't be improved by a LUT that takes true_position.w -> true_distance, 
					because it disagrees with the chord-based lookup in the light shader. This 
					result gets stored in the light map, and then the light map is checked against 
					the position in the G-buffer using the chord. The end result seems to be that 
					this distance is always smaller than the chord-computed distance, and all 
					surfaces are in shadow.

					Applying a bias just makes this value zig-zag above and below the chord-computed 
					value.

					A possible solution would be to draw only back faces during light map 
					generation, but then something has to be done about the ground having no 
					back faces.
				*/
				vec4 delta = true_position - vec4(0, 0, 0, 1);
				float true_distance_normalized = texture(chord2_lut, dot(delta, delta) * chord2_lut_scale + chord2_lut_offset).r;

				if(distance > 3.141593)
					true_distance_normalized = 1 - true_distance_normalized;

				gl_FragDepth = clamp(true_distance_normalized, 0, 1);
			}
		)",
		[](ShaderProgram* program) {
			program->set_lut("chord2_lut", 0, s_chord2_lut);
		},
		NULL,
		NULL,
		{
			new ShaderOption(DEFINE_VERTEX_COLOR),
			new ShaderOption(DEFINE_VERTEX_NORMAL),
			new ShaderOption(DEFINE_INSTANCED_BASE_COLOR),
			new ShaderOption(DEFINE_SHADOW)
		}
	);

	//Screenspace Shaders
	vert_screenspace = new ShaderCore(
		"vert_screenspace",
		GL_VERTEX_SHADER,
		R"(
			layout (location = 0) in vec4 position;
			layout (location = 1) in vec2 tex_coord;

			out vec2 vf_tex_coord;

			void main() {
				gl_Position = position;
				vf_tex_coord = tex_coord;
			}
		)",
		NULL,
		NULL,
		NULL,
		{}
	);
}
