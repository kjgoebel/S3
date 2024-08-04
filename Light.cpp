#include "Light.h"


void Light::render(DrawFunc draw_scene)
{
	use_lbuffer();
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, LIGHT_MAP_SIZE, LIGHT_MAP_SIZE);

	s_curcam = this;
	s_is_shadow_pass = true;
	draw_scene();
	s_is_shadow_pass = false;
	s_curcam = &cam;

	use_abuffer();
	glViewport(0, 0, window_width, window_height);

	ShaderProgram* light_program = ShaderProgram::get(
		Shader::get(vert_screenspace, {}),
		NULL,
		Shader::get(frag_point_light, {})
	);

	light_program->use();
	light_program->set_matrix("light_xform", ~mat * cam.get_mat());
	light_program->set_vector("light_pos", ~cam.get_mat() * mat.get_column(_w));
	light_program->set_vector("light_emission", emission);

	draw_fsq();
}


void Light::draw()
{
	model->draw(mat, 10 * emission);
}
