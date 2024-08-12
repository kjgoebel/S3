#include <stdlib.h>
#include "GL/glew.h"
#include "GL/glut.h"

#include "Model.h"
#include "Shaders.h"
#include "Utils.h"
#include "Framebuffer.h"
#include "Light.h"

#include <stdio.h>
#include <time.h>

#pragma warning(disable : 4244)		//conversion from double to float


#define PRINT_FRAME_RATE

#define NUM_DOTS		(2000)
#define NUM_BOULDERS	(40)

#define WALK_SPEED		(TAU / 50)		//Note that this isn't on the same scale as distances on the Sphere.
#define SUN_SPEED		(TAU / 60)

#define FOG_INCREMENT	(0.5)

enum Mode {
	NORMAL,
	COPY_TEXTURES,
	DUMP_LIGHT_MAP,
	DUMP_LUT,
	DUMP_BLOOM_MAIN_COLOR,
	DUMP_BLOOM_BRIGHT_COLOR,
	DUMP_BLOOM_RESULT,
	DUMP_INDEX_BUFFER,
	DUMP_LIGHT_INDEX_COUNT_MAP,
	DUMP_LIGHT_INDEX_MAP
};

bool bloom = true;


Model* dots_model;
Model* torus_model;
Model* boulder_model;
DrawFunc render_boulders;
ShaderProgram* final_program;
ShaderProgram *bloom_separate_program, *bloom_program_h, *bloom_program_v;
Mode mode = NORMAL;

Framebuffer *bloom_separate, *bloom_h, *bloom_v;
Pass *gpass, *apass, *unlit_pass, *bloom_separate_pass, *bloom_h_pass, *bloom_v_pass, *final_pass;

std::vector<Light*> lights;

double last_frame_time;

GLuint dump_image_layer = 0;


Mat4 sun_xform()
{
	return Mat4::axial_rotation(_x, _y, SUN_SPEED * last_frame_time) * Mat4::axial_rotation(_w, _x, TAU / 4) * Mat4::axial_rotation(_z, _y, TAU / 4);
}

Mat4 torus_world_xform(double x, double y, double z, double yaw, double pitch, double roll)
{
	double cx = cos(x), sx = sin(x), cy = cos(y), sy = sin(y);
	Vec4 pos = INV_ROOT_2 * Vec4(cx, sx, cy, sy);
	Vec4 fwd = Vec4(0, 0, -sy, cy);

	Vec4 down = INV_ROOT_2 * Vec4(-cx, -sx, cy, sy);
	Vec4 right = Vec4(sx, -cx, 0, 0);
	
	return Mat4::from_columns(right, down, fwd, pos)
				* Mat4::axial_rotation(_y, _w, z)
				* Mat4::axial_rotation(_z, _x, yaw)
				* Mat4::axial_rotation(_y, _z, pitch)
				* Mat4::axial_rotation(_x, _y, roll);
}


struct Controls
{
	bool	fwd, back,
			left, right;
};
Controls controls;

struct PlayerState
{
	double a, b;
	double yaw, pitch;

	void set_cam() const
	{
		cam.set_mat(torus_world_xform(b, a, 0.03, yaw, pitch, 0));
		double ca = cos(a), sa = sin(a), cb = cos(b), sb = sin(b);
	}
};
PlayerState player_state;


void init()
{
	check_gl_errors("init 0");
	
	glClearColor(0, 0, 0, 0);
	//glEnable(GL_PROGRAM_POINT_SIZE);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	init_random();
	init_luts();
	init_shaders();
	init_framebuffers();

	check_gl_errors("init 1");

	s_abuffer = new Screenbuffer("A-Buffer", {{GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT0}}, {{GL_DEPTH_ATTACHMENT, s_gbuffer_depth}});
	bloom_separate = new Screenbuffer(
		"Bloom Separate",
		{
			{GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT0},
			{GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT1}
		},
		{}
	);
	bloom_h = new Screenbuffer(
		"Bloom H",
		{{GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT0}},
		{}
	);
	bloom_v = new Screenbuffer(
		"Bloom V",
		{{GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT0}},
		{}
	);

	gpass = new Pass(s_gbuffer);

	/*
		This pass only exists to clear the A-buffer. All the actual drawing 
		into the A-buffer will be done by light passes and then the unlit 
		pass.
	*/
	apass = new Pass(s_abuffer);
	apass->clear_mask = GL_COLOR_BUFFER_BIT;

	unlit_pass = new Pass(s_abuffer);
	unlit_pass->clear_mask = 0;
	unlit_pass->depth_test = unlit_pass->depth_mask = true;
	unlit_pass->cull_face = GL_BACK;
	unlit_pass->blend = false;

	bloom_separate_pass = new Pass(bloom_separate);
	bloom_separate_pass->clear_mask = 0;
	bloom_separate_pass->depth_test = bloom_separate_pass->depth_mask = false;
	bloom_separate_pass->cull_face = 0;
	bloom_separate_pass->blend = false;

	bloom_h_pass = new Pass(bloom_h, bloom_separate_pass);

	bloom_v_pass = new Pass(bloom_v, bloom_separate_pass);

	final_pass = new Pass(NULL);
	final_pass->clear_mask = 0;
	final_pass->depth_test = final_pass->depth_mask = false;
	final_pass->cull_face = 0;
	final_pass->blend = false;

	check_gl_errors("init 2");

	bloom_separate_program = ShaderProgram::get(
		Shader::get(vert_screenspace, {}),
		NULL,
		Shader::get(frag_bloom_separate, {})
	);

	bloom_program_h = ShaderProgram::get(
		Shader::get(vert_screenspace, {}),
		NULL,
		Shader::get(frag_bloom, {DEFINE_HORIZONTAL})
	);

	bloom_program_v = ShaderProgram::get(
		Shader::get(vert_screenspace, {}),
		NULL,
		Shader::get(frag_bloom, {})
	);

	final_program = ShaderProgram::get(
		Shader::get(vert_screenspace, {}),
		NULL,
		Shader::get(frag_final_color, {})
	);

	check_gl_errors("init 3");

	//Note: Should make half as many dots but only on the right side of the torus.
	Vec4* dots = new Vec4[NUM_DOTS];
	for(int i = 0; i < NUM_DOTS; i++)
	{
		double c, s;
		do {
			dots[i] = rand_s3();
			s = sqrt(1 - dots[i].w * dots[i].w - dots[i].z * dots[i].z);
			c = sqrt(1 - dots[i].y * dots[i].y - dots[i].x * dots[i].x);
		} while(c > s);
	}
	dots_model = new Model(NUM_DOTS, dots);
	delete[] dots;

	torus_model = Model::make_bumpy_torus(50, 50, 0.03);
	torus_model->triangulate();
	torus_model->generate_primitive_colors(0.7);
	torus_model->generate_normals();

	boulder_model =  Model::make_icosahedron(0.1, 1);
	boulder_model->generate_primitive_colors(0.3);
	boulder_model->generate_normals();

	Mat4* boulders = new Mat4[NUM_BOULDERS];
	for(int i = 0; i < NUM_BOULDERS; i++)
		boulders[i] = torus_world_xform(frand() * TAU, frand() * TAU, 0.05, frand() * TAU, fsrand() * 0.5 * TAU, fsrand() * 0.5 * TAU);
	render_boulders = boulder_model->make_draw_func(
		NUM_BOULDERS,
		boulders,
		Vec4(0.7, 0.7, 0.7, 1),
		false,
		torus_model->get_num_vertices() + dots_model->get_num_vertices()
	);
	delete[] boulders;

	check_gl_errors("init 4");

	//The Sun
	lights.push_back(new Light(
		sun_xform(),
		Vec3(1, 1, 1),
		Model::make_icosahedron(0.05, 2, true)
	));

	Model* light_model = Model::make_icosahedron(0.02, 2, true);

	//The Unlight
	/*
	lights.push_back(new Light(
		Mat4::axial_rotation(_w, _x, TAU / 6),
		-Vec3(0.6, 0.6, 0.6),
		light_model
	));

	//Generic lights
	for(int i = 0; i < 4; i++)
		lights.push_back(new Light(
			torus_world_xform(frand() * TAU, frand() * TAU, 0.15 + 0.15 * frand(), 0, 0, 0),
			0.25 * Vec3(frand(), frand(), frand()),
			light_model
		));
	*/

	check_gl_errors("init 5");
	
	player_state.a = player_state.b = player_state.yaw = player_state.pitch = 0;
	player_state.set_cam();
}

void reshape(int w, int h)
{
	resize_screenbuffers(w, h);
	cam.set_perspective((double)window_width / window_height);

	ShaderProgram::init_all();
}

void draw_scene()
{
	torus_model->draw(Mat4::identity(), Vec4(0.3, 0.3, 0.3, 1), 0);
	dots_model->draw(Mat4::identity(), Vec4(1, 1, 1, 1), torus_model->get_num_vertices());
	render_boulders();
}

void display()
{
	check_gl_errors("display 0");

	glutWarpPointer(window_width >> 1, window_height >> 1);

	double dt = current_time() - last_frame_time;

	#define CONTROL_SPEED(positive, negative, speed)	(\
		(positive && !negative) ? speed * dt : \
			(negative && !positive) ? -speed * dt : 0\
	)
	
	double	fwd = CONTROL_SPEED(controls.fwd, controls.back, WALK_SPEED),
			right = CONTROL_SPEED(controls.left, controls.right, WALK_SPEED),
			cy = cos(player_state.yaw),
			sy = sin(player_state.yaw);
	player_state.a += fwd * cy + right * sy;
	player_state.b += right * cy - fwd * sy;

	player_state.set_cam();

	last_frame_time += dt;

	#ifdef PRINT_FRAME_RATE
		printf("%f\n", 1.0 / dt);
		print_matrix(cam.get_mat());
		printf("\n");
	#endif

	check_gl_errors("display 1");

	ShaderProgram::frame_all();

	check_gl_errors("display 2");

	//Geometry Pass
	gpass->start();
	draw_scene();

	check_gl_errors("display 3");
	
	//Light (Accumulation) Pass
	apass->start();
	lights[0]->set_mat(sun_xform());

	for(auto light : lights)
		light->render(draw_scene);

	check_gl_errors("display 4");
	
	//Unlit Pass
	unlit_pass->start();

	for(auto light : lights)
		light->draw();

	check_gl_errors("display 5");
	
	if(bloom)
	{
		//Bloom Passes
		bloom_separate_pass->start();
		bloom_separate_program->use();
		bloom_separate_program->set_texture("color_tex", 0, s_abuffer_color);
		draw_fsq();

		bloom_h_pass->start();
		bloom_program_h->use();
		bloom_program_h->set_texture("color_tex", 0, bloom_separate->textures[1]);
		draw_fsq();

		bloom_v_pass->start();
		bloom_program_v->use();
		bloom_program_v->set_texture("color_tex", 0, bloom_h->textures[0]);
		draw_fsq();

		//Final Pass
		final_pass->start();
		final_program->set_texture("main_color_tex", 0, bloom_separate->textures[0]);
		final_program->set_texture("bright_color_tex", 1, bloom_v->textures[0]);
	}
	else
	{
		//Final Pass
		final_pass->start();
		final_program->set_texture("main_color_tex", 0, s_abuffer_color);
		final_program->set_texture("bright_color_tex", 1, 0);		//This is naughty.
	}

	switch(mode)
	{
		case NORMAL:
			final_program->use();
			draw_fsq();
			break;

		case COPY_TEXTURES:
			{
				ShaderProgram* dump_program = ShaderProgram::get(
					Shader::get(vert_screenspace, {}),
					NULL,
					Shader::get(frag_dump_texture, {})
				);

				dump_program->use();
			
				dump_program->set_texture("tex", 0, s_gbuffer_albedo);
				draw_qsq(0);
				dump_program->set_texture("tex", 0, s_gbuffer_position);
				draw_qsq(1);
				dump_program->set_texture("tex", 0, s_gbuffer_normal);
				draw_qsq(2);
				dump_program->set_texture("tex", 0, s_gbuffer_depth);
				draw_qsq(3);
			}
			break;

		case DUMP_LIGHT_MAP:
			{
				ShaderProgram* dump_cube_program = ShaderProgram::get(
					Shader::get(vert_screenspace, {}),
					NULL,
					Shader::get(frag_dump_cubemap, {})
				);
				glClear(GL_COLOR_BUFFER_BIT);
				dump_cube_program->use();
				dump_cube_program->set_texture("tex", 0, lights[0]->shadow_map(), GL_TEXTURE_CUBE_MAP);
				dump_cube_program->set_float("z_mult", 1);
				draw_hsq(0);
				dump_cube_program->set_float("z_mult", -1);
				draw_hsq(1);
			}
			break;

		case DUMP_LUT:
			{
				ShaderProgram* dump_program = ShaderProgram::get(
					Shader::get(vert_screenspace, {}),
					NULL,
					Shader::get(frag_dump_texture1d, {})
				);
				dump_program->use();
				dump_program->set_texture("tex", 0, s_chord2_lut->get_texture(), s_chord2_lut->get_target());
				dump_program->set_float("output_scale", 0.01);
				dump_program->set_float("output_offset", 0);
				draw_fsq();
			}
			break;

		case DUMP_INDEX_BUFFER:
			{
				ShaderProgram* dump_program = ShaderProgram::get(
					Shader::get(vert_screenspace, {}),
					NULL,
					Shader::get(frag_dump_texture, {DEFINE_INTEGER_TEX})
				);
				dump_program->use();
				dump_program->set_texture("tex", 0, s_gbuffer_index);
				draw_fsq();
			}
			break;
			
		case DUMP_LIGHT_INDEX_COUNT_MAP:
			/*{
				ShaderProgram* dump_cube_program = ShaderProgram::get(
					Shader::get(vert_screenspace, {}),
					NULL,
					Shader::get(frag_dump_cubemap, {DEFINE_INTEGER_TEX})
				);
				glClear(GL_COLOR_BUFFER_BIT);
				dump_cube_program->use();
				dump_cube_program->set_texture("tex", 0, lights[0]->get_index_count_map(), GL_TEXTURE_CUBE_MAP);
				dump_cube_program->set_float("sample_mult", 32);
				dump_cube_program->set_float("z_mult", 1);
				draw_hsq(0);
				dump_cube_program->set_float("z_mult", -1);
				draw_hsq(1);
			}
			break;*/

		case DUMP_LIGHT_INDEX_MAP:
			{
				ShaderProgram* dump_cube_program = ShaderProgram::get(
					Shader::get(vert_screenspace, {}),
					NULL,
					Shader::get(frag_dump_image_cube_array, {})
				);
				glClear(GL_COLOR_BUFFER_BIT);
				dump_cube_program->use();
				if(mode == DUMP_LIGHT_INDEX_COUNT_MAP)
				{
					dump_cube_program->set_image_texture(0, lights[0]->get_index_count_map(), GL_READ_ONLY, GL_R32UI);
					dump_cube_program->set_uint("sample_mult", 32);
				}
				else
				{
					dump_cube_program->set_image_texture(0, lights[0]->get_index_map(), GL_READ_ONLY, GL_R32UI);
					dump_cube_program->set_uint("sample_mult", 1);
				}
				dump_cube_program->set_uint("layer", dump_image_layer);
				draw_fsq();
			}
			break;

		default:
			{
				ShaderProgram* dump_program = ShaderProgram::get(
					Shader::get(vert_screenspace, {}),
					NULL,
					Shader::get(frag_dump_texture, {})
				);
				dump_program->use();
				switch(mode) 
				{
					case DUMP_BLOOM_RESULT:
						dump_program->set_texture("tex", 0, bloom_h->textures[0]);
						break;
					case DUMP_BLOOM_MAIN_COLOR:
						dump_program->set_texture("tex", 0, bloom_separate->textures[0]);
						break;
					case DUMP_BLOOM_BRIGHT_COLOR:
						dump_program->set_texture("tex", 0, bloom_separate->textures[1]);
						break;
				}
				draw_fsq();
			}
			break;
	}
	
	check_gl_errors("display 6");

	glFlush();
	glutSwapBuffers();
	glutPostRedisplay();

	check_gl_errors("display 7");
}

#define PITCH_SENSITIVITY (0.001)
#define YAW_SENSITIVITY (0.001)

void mouse(int x, int y)
{
	y -= window_height >> 1;
	x -= window_width >> 1;
	player_state.pitch -= PITCH_SENSITIVITY * y;
	player_state.yaw += YAW_SENSITIVITY * x;

	while(player_state.yaw > TAU)
		player_state.yaw -= TAU;
	while(player_state.yaw < 0)
		player_state.yaw += TAU;

	if(player_state.pitch > TAU / 4)
		player_state.pitch = TAU / 4;
	if (player_state.pitch < -TAU / 4)
		player_state.pitch = -TAU / 4;
}

void keyboard(unsigned char key, int x, int y)
{
	switch(key)
	{
		case 27:
			exit(0);
			break;

		case 'w':
			controls.fwd = true;
			break;
		case 's':
			controls.back = true;
			break;
		case 'a':
			controls.left = true;
			break;
		case 'd':
			controls.right = true;
			break;

		case '[':
			s_fog_scale -= FOG_INCREMENT;
			if(s_fog_scale < 0.0)
				s_fog_scale = 0.0;
			break;
		case ']':
			s_fog_scale += FOG_INCREMENT;
			break;
	}
}

void keyboard_up(unsigned char key, int x, int y)
{
	switch(key)
	{
		case 'w':
			controls.fwd = false;
			break;
		case 's':
			controls.back = false;
			break;
		case 'a':
			controls.left = false;
			break;
		case 'd':
			controls.right = false;
			break;

		case '[':
			if(dump_image_layer)
				dump_image_layer--;
			break;
		case ']':
			dump_image_layer++;
			break;
	}
}

void special(int key, int x, int y)
{
	switch(key)
	{
		case GLUT_KEY_F1:
			mode = NORMAL;
			break;
		case GLUT_KEY_F2:
			mode = COPY_TEXTURES;
			break;
		case GLUT_KEY_F3:
			mode = DUMP_LIGHT_MAP;
			break;
		case GLUT_KEY_F4:
			mode = DUMP_LUT;
			break;

		case GLUT_KEY_F5:
			mode = DUMP_BLOOM_RESULT;
			break;
		case GLUT_KEY_F6:
			mode = DUMP_BLOOM_MAIN_COLOR;
			break;
		case GLUT_KEY_F7:
			mode = DUMP_BLOOM_BRIGHT_COLOR;
			break;
		case GLUT_KEY_F8:
			bloom = !bloom;
			break;
		
		case GLUT_KEY_F9:
			mode = DUMP_INDEX_BUFFER;
			break;
		case GLUT_KEY_F11:
			mode = DUMP_LIGHT_INDEX_COUNT_MAP;
			break;
		case GLUT_KEY_F12:
			mode = DUMP_LIGHT_INDEX_MAP;
			break;
	}
}


int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(1600, 900);
	glutInitWindowPosition(160,  90);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

	glutCreateWindow("WindowName");

	glewInit();

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyboard_up);
	glutSpecialFunc(special);
	glutPassiveMotionFunc(mouse);
	glutMotionFunc(mouse);

	init();

	glutSetCursor(GLUT_CURSOR_NONE);

	glutMainLoop();
	return 0;
}
