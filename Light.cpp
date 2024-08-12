#include "Light.h"
#include "Utils.h"


#define SHADOW_MAP_SIZE	(2048)
#define NUM_INDEX_LAYERS (8)


Screenbuffer* s_abuffer;
TextureSpec Light::index_count_spec = {GL_TEXTURE_CUBE_MAP, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, GL_NONE, GL_NEAREST, GL_NEAREST};
TextureSpec Light::index_spec = {GL_TEXTURE_CUBE_MAP_ARRAY, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, GL_NONE, GL_NEAREST, GL_NEAREST};


Light::Light(Mat4& mat, Vec3& emission, Model* model, double near_clip)
	: Camera(mat, 1, TAU / 4, near_clip), emission(emission), model(model)
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
	
	index_count_map = index_count_spec.make_texture(0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	index_map = index_spec.make_texture(0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, NUM_INDEX_LAYERS);

	check_gl_errors("Light::Light() 2");

	shadow_pass = new Pass(shadow_buffer);
	shadow_pass->clear_mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
	shadow_pass->is_shadow_pass = true;

	check_gl_errors("Light::Light() 3");

	light_pass = new Pass(s_abuffer);
	light_pass->clear_mask = 0;
	light_pass->depth_test = light_pass->depth_mask = false;
	light_pass->blend = true;		//specifically GL_FUNC_ADD and GL_ONE, GL_ONE
	
	check_gl_errors("Light::Light() 4");

	shadow_map_dirty = true;
}

void Light::render(DrawFunc draw_scene)
{
	if(shadow_map_dirty)
	{
		shadow_pass->start();

		s_curcam = this;
		glEnable(GL_CONSERVATIVE_RASTERIZATION_NV);
		GLuint temp = 0;
		check_gl_errors("Light::render() before clearing");
		glClearTexImage(index_count_map, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &temp);
		glClearTexImage(index_map, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &temp);
		check_gl_errors("Light::render() after clearing");
		draw_scene();
		glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
		s_curcam = &cam;

		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		shadow_map_dirty = false;
	}

	light_pass->start();

	ShaderProgram* light_program = ShaderProgram::get(
		Shader::get(vert_screenspace, {}),
		NULL,
		Shader::get(frag_point_light, {DEFINE_USE_PRIM_INDEX})
	);
	
	check_gl_errors("Light::render() before use()\n");
	light_program->use();
	check_gl_errors("Light::render() after use()\n");
	light_program->set_matrix("light_xform", ~mat * cam.get_mat());
	light_program->set_vector("light_pos", ~cam.get_mat() * mat.get_column(_w));
	light_program->set_vector("light_emission", emission);
	light_program->set_texture("shadow_map", 3, shadow_map(), GL_TEXTURE_CUBE_MAP);
	light_program->set_texture("light_index_count_map", 6, get_index_count_map(), GL_TEXTURE_CUBE_MAP);
	light_program->set_texture("light_index_map", 7, get_index_map(), GL_TEXTURE_CUBE_MAP_ARRAY);
	light_program->set_float("light_index_map_scale", 1.5 / SHADOW_MAP_SIZE);

	check_gl_errors("Light::render() before draw_fsq()\n");
	draw_fsq();
	check_gl_errors("Light::render() after draw_fsq()\n");
}


void Light::draw()
{
	model->draw(mat, 10 * emission);
}
