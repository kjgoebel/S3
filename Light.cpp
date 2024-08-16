#include "Light.h"

#include "Utils.h"
#include "TorusWorldShaders.h"


#define SHADOW_MAP_SIZE	(4096)


double s_fog_density = 0.1;
Vec3 s_fog_color(1, 1, 1);


Light::Light(Mat4& mat, Vec3& emission, Model* model, bool use_fog, double near_clip)
	: Camera(mat, 1, TAU / 4, near_clip), emission(emission), model(model), use_fog(use_fog)
{
	check_gl_errors("Light::Light() 0");

	shadow_buffer = new Framebuffer(
		"Shadow Buffer",
		{{GL_TEXTURE_CUBE_MAP, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_ATTACHMENT, GL_LINEAR, GL_LINEAR}},
		{},
		SHADOW_MAP_SIZE,
		SHADOW_MAP_SIZE
	);

	check_gl_errors("Light::Light() 1");
	
	shadow_pass = new Pass(shadow_buffer);
	shadow_pass->clear_mask = GL_DEPTH_BUFFER_BIT;
	shadow_pass->is_shadow_pass = true;

	check_gl_errors("Light::Light() 2");

	light_pass = new Pass(s_abuffer);
	light_pass->clear_mask = 0;
	light_pass->depth_test = light_pass->depth_mask = false;
	light_pass->blend = true;		//specifically GL_FUNC_ADD and GL_ONE, GL_ONE
	
	check_gl_errors("Light::Light() 3");

	shadow_map_dirty = true;
}

void Light::render(DrawFunc draw_scene)
{
	if(shadow_map_dirty)
	{
		shadow_pass->start();

		s_curcam = this;
		draw_scene();
		s_curcam = &cam;

		shadow_map_dirty = false;
	}

	light_pass->start();

	std::set<const char*> frag_options;
	if(use_fog)
		frag_options.insert(DEFINE_USE_FOG);
	ShaderProgram* light_program = ShaderProgram::get(
		Shader::get(vert_screenspace, {}),
		NULL,
		Shader::get(frag_point_light, frag_options)
	);

	light_program->use();
	light_program->set_matrix("light_xform", ~mat * cam.get_mat());
	light_program->set_vector("light_pos", ~cam.get_mat() * mat.get_column(_w));
	light_program->set_vector("light_emission", emission);
	light_program->set_texture("light_map", 5, shadow_map(), GL_TEXTURE_CUBE_MAP);

	draw_fsq();
}


void Light::draw()
{
	model->draw(mat, 10 * emission);
}
