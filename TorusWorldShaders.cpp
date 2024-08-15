#include "TorusWorldShaders.h"


ShaderCore *frag_point_light, *frag_bloom, *frag_bloom_separate, *frag_final_color;
ShaderCore *frag_copy_textures, *frag_dump_texture, *frag_dump_cubemap, *frag_dump_texture1d;

Screenbuffer* s_abuffer;


void init_torus_world_shaders()
{
	s_abuffer = new Screenbuffer("A-Buffer", {{GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT0}}, {{GL_DEPTH_ATTACHMENT, s_gbuffer_depth}});

	frag_point_light = new ShaderCore(
		"frag_point_light",
		GL_FRAGMENT_SHADER,
		R"(
			uniform sampler1D chord2_lut;
			uniform float chord2_lut_scale;
			uniform float chord2_lut_offset;

			uniform sampler2D albedo_tex;
			uniform sampler2D position_tex;
			uniform sampler2D normal_tex;
			uniform sampler2D depth_tex;
			uniform samplerCube light_map;

			uniform mat4 light_xform;
			uniform vec4 light_pos;
			uniform vec3 light_emission;

			#define NUM_SHADOW_SAMPLES 5
			#define SHADOW_SAMPLE_EXTENT 0.001
			float shadow_offset(int i) {
				return -SHADOW_SAMPLE_EXTENT + i * 2 * SHADOW_SAMPLE_EXTENT / (NUM_SHADOW_SAMPLES - 1);
			}

			#ifdef USE_FOG
				#define NUM_FOG_STEPS (50)
				uniform float fog_density = 0.3;
				uniform vec3 fog_color = vec3(1, 1, 1);
			#endif

			out vec4 frag_color;

			void main() {
				ivec2 pixel_coords = ivec2(gl_FragCoord.xy);
				vec4 albedo = texelFetch(albedo_tex, pixel_coords, 0);
				vec4 position = texelFetch(position_tex, pixel_coords, 0);
				vec4 normal = texelFetch(normal_tex, pixel_coords, 0);

				vec4 lightspace_pos = light_xform * position;
				vec4 lightspace_delta = lightspace_pos - vec4(0, 0, 0, 1);

				//DISTANCE:
				vec4 lut_data = texture(chord2_lut, dot(lightspace_delta, lightspace_delta) * chord2_lut_scale + chord2_lut_offset);
				float distance_factor = lut_data.y;		//1 / sin^2(distance)'

				//NORMAL:
				/*
					Dot product between the surface normal and the far image of the light.
					If this is positive, we can see the far image of the light. If it's 
					negative, we can see the near image. Either way, we are illuminated 
					according to the magnitude of the dot product (if we're not in shadow).
				*/
				vec4 lightspace_normal = light_xform * normal;
				float long_dot = dot(lightspace_normal, lightspace_delta);
				float normal_factor = abs(long_dot);

				//SHADOW:
				if(long_dot > 0)			//If we're facing the far image of the light, check the complimentary distance in the opposite direction.
				{
					lightspace_pos.xyz = -lightspace_pos.xyz;
					lut_data.x = 1 - lut_data.x;		//normalized distance
				}
				float shadow_factor = 0.0;
				for(int i = 0; i < NUM_SHADOW_SAMPLES; i++)
				{
					float distance_delta = texture(light_map, lightspace_pos.xyz + shadow_offset(i) * lightspace_normal.xyz).r - lut_data.x;
					shadow_factor += smoothstep(-0.002, 0.0, distance_delta);
				}
				shadow_factor /= NUM_SHADOW_SAMPLES;

				frag_color.rgb = shadow_factor * normal_factor * distance_factor * light_emission * albedo.rgb;
				frag_color.a = 1;

				#ifdef USE_FOG
					//FOG:
					vec4 ortho_position;		//Position orthogonalized against the camera position.
					ortho_position.xyz = normalize(position.xyz);
					ortho_position.w = 1;
					float distance = texelFetch(depth_tex, pixel_coords, 0).r * 6.283185;

					vec3 fog = vec3(0);
					for(int i = 1; i < NUM_FOG_STEPS; i++)
					{
						float theta = i * distance / NUM_FOG_STEPS;
						vec4 curpos = cos(theta) * vec4(0, 0, 0, 1) + sin(theta) * ortho_position;
						//Note: reusing variable from before. I don't like this.
						lightspace_delta = light_xform * curpos;
						lightspace_delta.w -= 1;
						float lightspace_distance = texture(chord2_lut, dot(lightspace_delta, lightspace_delta) * chord2_lut_scale + chord2_lut_offset).x;
						if(lightspace_distance < texture(light_map, lightspace_delta.xyz).r)
							fog += distance * fog_color;
						if(1 - lightspace_distance < texture(light_map, -lightspace_delta.xyz).r)
							fog += distance * fog_color;
					}
					fog /= NUM_FOG_STEPS;
					
					frag_color.xyz += (1 - exp(-distance * fog_density)) * fog;
				#endif
			}
		)",
		[](ShaderProgram* program) {
			program->set_lut("chord2_lut", 4, s_chord2_lut);
		},
		NULL,
		[](ShaderProgram* program) {
			program->set_texture("albedo_tex", 0, s_gbuffer_albedo);
			program->set_texture("position_tex", 1, s_gbuffer_position);
			program->set_texture("normal_tex", 2, s_gbuffer_normal);
		},
		{
			new ShaderOption(
				DEFINE_USE_FOG,
				NULL,
				NULL,
				[](ShaderProgram* program) {
					program->set_texture("depth_tex", 3, s_gbuffer_depth);
				}
			)
		}
	);

	frag_bloom_separate = new ShaderCore(
		"frag_bloom_separate",
		GL_FRAGMENT_SHADER,
		R"(
			uniform sampler2D color_tex;

			layout (location = 0) out vec4 main_frag_color;
			layout (location = 1) out vec4 bright_frag_color;

			void main() {
				ivec2 pixel_coords = ivec2(gl_FragCoord.xy);
				vec3 input_color = texelFetch(color_tex, pixel_coords, 0).rgb;
				float value = abs(max(input_color.x, max(input_color.y, input_color.z)));
				float main_fraction = 1 / max(value, 1);
				main_frag_color.rgb = input_color * main_fraction;
				bright_frag_color.rgb = input_color * (1 - main_fraction);
				bright_frag_color.a = main_frag_color.a = 1;
			}
		)",
		NULL,
		NULL,
		NULL,
		{}
	);

	frag_bloom = new ShaderCore(
		"frag_bloom",
		GL_FRAGMENT_SHADER,
		R"(
			uniform sampler2D color_tex;

			uniform float sample_strength[8] = {1, 0.960789439152, 0.852143788966, 0.697676326071, 0.527292424043, 0.367879441171, 0.236927758682, 0.140858420921};
			uniform float normalization_constant = 0.114655958964;

			#ifdef HORIZONTAL
				#define direction (ivec2(1, 0))
			#else
				#define direction (ivec2(0, 1))
			#endif

			out vec4 frag_color;

			void main() {
				ivec2 pixel_coords = ivec2(gl_FragCoord.xy);
				vec3 color = vec3(0);

				for(int i = -7; i <= 7; i++)
					color += sample_strength[abs(i)] * texelFetch(color_tex, pixel_coords + i * direction, 0).rgb;

				color *= normalization_constant;

				frag_color.rgb = color;
				frag_color.a = 1;
			}
		)",
		NULL,
		NULL,
		NULL,
		{
			new ShaderOption(DEFINE_HORIZONTAL)
		}
	);

	frag_final_color = new ShaderCore(
		"frag_final_color",
		GL_FRAGMENT_SHADER,
		R"(
			uniform sampler2D main_color_tex;
			uniform sampler2D bright_color_tex;

			out vec4 frag_color;

			void main() {
				ivec2 pixel_coords = ivec2(gl_FragCoord.xy);
				vec3 color = texelFetch(main_color_tex, pixel_coords, 0).rgb + texelFetch(bright_color_tex, pixel_coords, 0).rgb;
				float value = max(color.r, max(color.g, color.b));
				frag_color.rgb = tanh(value) * color / value;
				frag_color.a = 1;
			}
		)",
		NULL,
		NULL,
		NULL,
		{}
	);
	
	//Debugging Shaders
	frag_dump_texture = new ShaderCore(
		"frag_dump_texture",
		GL_FRAGMENT_SHADER,
		R"(
			uniform sampler2D tex;

			in vec2 vf_tex_coord;

			out vec4 frag_color;

			void main() {
				frag_color = texture(tex, vf_tex_coord);
			}
		)",
		NULL,
		NULL,
		NULL,
		{}
	);

	frag_dump_cubemap = new ShaderCore(
		"frag_dump_cubemap",
		GL_FRAGMENT_SHADER,
		R"(
			uniform samplerCube tex;
			
			uniform float z_mult;

			in vec2 vf_tex_coord;

			out vec4 frag_color;

			void main() {
				vec2 coord = vf_tex_coord * 2 - 1;
				float temp = 1 - coord.x * coord.x - coord.y * coord.y;
				if(temp < 0)
					discard;
				vec3 dir = vec3(coord.x, coord.y, z_mult * sqrt(temp));
				frag_color = texture(tex, dir);
			}
		)",
		NULL,
		NULL,
		NULL,
		{}
	);

	frag_dump_texture1d = new ShaderCore(
		"frag_dump_texture1d",
		GL_FRAGMENT_SHADER,
		R"(
			uniform sampler1D tex;
			uniform float output_scale, output_offset;

			in vec2 vf_tex_coord;

			out vec4 frag_color;

			void main() {
				frag_color = texture(tex, vf_tex_coord.x) * output_scale + output_offset;
			}
		)",
		NULL,
		NULL,
		NULL,
		{}
	);
}
