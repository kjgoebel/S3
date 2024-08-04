#include "Light.h"


#define LIGHT_MAP_SIZE	(4096)


Light::Light(Mat4& mat, Vec3& emission, Model* model, double near_clip)
	: Camera(mat, 1, TAU / 4, near_clip), emission(emission), model(model)
{
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glDrawBuffers(0, NULL);

	glGenTextures(1, &shadow_map);
	glBindTexture(GL_TEXTURE_CUBE_MAP, shadow_map);
	for(int face = 0; face < 6; face++)
		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
			0,
			GL_DEPTH_COMPONENT32F,
			LIGHT_MAP_SIZE,
			LIGHT_MAP_SIZE,
			0,
			GL_DEPTH_COMPONENT,
			GL_FLOAT,
			NULL
		);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glNamedFramebufferTexture(framebuffer, GL_DEPTH_ATTACHMENT, shadow_map, 0);

	if(glCheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		error("L-Buffer status is %d\n", glCheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER));

	shadow_map_dirty = true;
}

void Light::render(DrawFunc draw_scene)
{
	if(shadow_map_dirty)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
		glClear(GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, LIGHT_MAP_SIZE, LIGHT_MAP_SIZE);

		s_curcam = this;
		s_is_shadow_pass = true;
		draw_scene();
		s_is_shadow_pass = false;
		s_curcam = &cam;

		use_abuffer();
		glViewport(0, 0, window_width, window_height);

		shadow_map_dirty = false;
	}

	ShaderProgram* light_program = ShaderProgram::get(
		Shader::get(vert_screenspace, {}),
		NULL,
		Shader::get(frag_point_light, {})
	);

	light_program->use();
	light_program->set_matrix("light_xform", ~mat * cam.get_mat());
	light_program->set_vector("light_pos", ~cam.get_mat() * mat.get_column(_w));
	light_program->set_vector("light_emission", emission);
	light_program->set_texture("light_map", 3, shadow_map, GL_TEXTURE_CUBE_MAP);

	draw_fsq();
}


void Light::draw()
{
	model->draw(mat, 10 * emission);
}
