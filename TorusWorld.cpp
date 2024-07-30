#include <stdlib.h>
#include "GL/glew.h"
#include "GL/glut.h"

#include "Model.h"
#include "Shaders.h"
#include "Utils.h"
#include "Framebuffer.h"

#include "PoleModel.h"

#include <stdio.h>
#include <time.h>

#pragma warning(disable : 4244)		//conversion from double to float


#define NUM_DOTS		(2000)
#define NUM_BOULDERS	(40)
#define LIGHT_MAP_SIZE	(4096)
#define WALK_SPEED		(TAU / 50)
#define FOG_INCREMENT	(0.5)
#define SUN_SPEED		(TAU / 30)

enum Mode {
	NORMAL,
	COPY_TEXTURES,
	DUMP_ALBEDO,
	DUMP_POSITION,
	DUMP_NORMAL,
	DUMP_DEPTH,
	DUMP_LIGHT_MAP
};


Model* dots_model;
Model* torus_model;
Model* pole_model;
DrawFunc render_boulders;
ShaderProgram *dump_program, *dump_cube_program, *light_program, *final_program;
Mode mode = NORMAL;

double last_frame_time;


Mat4& torus_world_xform(double x, double y, double z, double yaw, double pitch, double roll)
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
		cam_mat = torus_world_xform(b, a, 0.03, yaw, pitch, 0);
		double ca = cos(a), sa = sin(a), cb = cos(b), sb = sin(b);
	}
};
PlayerState player_state;


void init()
{
	srand(clock());

	glCullFace(GL_BACK);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	init_shaders();

	/*copy_program = ShaderProgram::get(
		Shader::get(vert_screenspace, {}),
		NULL,
		Shader::get(frag_copy_textures, {})
	);*/

	dump_program = ShaderProgram::get(
		Shader::get(vert_screenspace, {}),
		NULL,
		Shader::get(frag_dump_texture, {})
	);

	dump_cube_program = ShaderProgram::get(
		Shader::get(vert_screenspace, {}),
		NULL,
		Shader::get(frag_dump_cubemap, {})
	);

	light_program = ShaderProgram::get(
		Shader::get(vert_screenspace, {}),
		NULL,
		Shader::get(frag_point_light, {})
	);

	final_program = ShaderProgram::get(
		Shader::get(vert_screenspace, {}),
		NULL,
		Shader::get(frag_dump_color, {})
	);

	Vec4* dots = new Vec4[NUM_DOTS];
	for(int i = 0; i < NUM_DOTS; i++)
		dots[i] = rand_s3();
	dots_model = new Model(NUM_DOTS, dots);
	delete[] dots;

	torus_model = Model::make_bumpy_torus(64, 64, 0.03);
	torus_model->generate_normals();
	torus_model->generate_primitive_colors(0.7);

	pole_model =  new Model(
		GL_TRIANGLES,
		NUM_POLE_VERTS,
		3,
		NUM_POLE_TRIANGLES,
		Model::s3ify(NUM_POLE_VERTS, 0.1, pole_model_vertices).get(),
		pole_model_elements
	);
	pole_model->generate_primitive_colors(0.3);
	pole_model->generate_normals();

	Mat4* boulders = new Mat4[NUM_BOULDERS];
	for(int i = 0; i < NUM_BOULDERS; i++)
		boulders[i] = torus_world_xform(frand() * TAU, frand() * TAU, 0.05, frand() * TAU, fsrand() * 0.5 * TAU, fsrand() * 0.5 * TAU);
	render_boulders = pole_model->make_draw_func(NUM_BOULDERS, boulders, Vec4(0.7, 0.7, 0.7, 1));
	delete[] boulders;

	player_state.a = player_state.b = player_state.yaw = player_state.pitch = 0;
	player_state.set_cam();
}

int window_width = 0, window_height = 0;

void reshape(int w, int h)
{
	if(w != window_width || h != window_height)
	{
		window_width = w;
		window_height = h;
		init_framebuffer(window_width, window_height, LIGHT_MAP_SIZE);
	}

	ShaderProgram::init_all();
}

void draw_scene(bool shadow)
{
	is_shadow_pass = shadow;

	torus_model->draw(Mat4::identity(), Vec4(0.3, 0.3, 0.3, 1));
	dots_model->draw(Mat4::identity(), Vec4(1, 1, 1, 1));
	pole_model->draw(Mat4::axial_rotation(_w, _x, TAU / 4), Vec4(0.7, 0, 0, 1));
	pole_model->draw(Mat4::axial_rotation(_w, _y, TAU / 4), Vec4(0, 0.7, 0, 1));
	render_boulders();

	is_shadow_pass = false;
}

void render_point_light(Mat4& light_mat, Vec3 light_emission)
{
	use_lbuffer();
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, LIGHT_MAP_SIZE, LIGHT_MAP_SIZE);
	
	set_perspective(1);

	//This is filthy. This is why we don't communicate through globals.
	Mat4 temp = cam_mat;
	cam_mat = light_mat;
	draw_scene(true);
	cam_mat = temp;

	use_abuffer();
	glViewport(0, 0, window_width, window_height);
	light_program->use();
	light_program->set_matrix("light_xform", ~light_mat * cam_mat);
	light_program->set_vector("light_pos", ~cam_mat * light_mat.get_column(_w));
	light_program->set_vector("light_emission", light_emission);
	//light_program->set_texture("light_map", 3, light_map);

	draw_fsq();
}

void display()
{
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
	printf("%f\n", 1.0 / dt);
	print_matrix(cam_mat);
	printf("\n");

	ShaderProgram::frame_all();

	//Geometry Pass
	use_gbuffer();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, window_width, window_height);
	set_perspective((double)window_width / window_height);
	glDisable(GL_BLEND);
	draw_scene(false);

	//Light (Accumulation) Pass
	use_abuffer();

	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	
	render_point_light(
		Mat4::axial_rotation(_w, _x, TAU / 7),
		-Vec3(0.6, 0.2, 0.0)
	);
	render_point_light(
		Mat4::axial_rotation(_x, _y, SUN_SPEED * last_frame_time) * Mat4::axial_rotation(_w, _x, TAU / 4) * Mat4::axial_rotation(_z, _y, TAU / 4),
		Vec3(1, 1, 1)
	);
	/*render_point_light(
		cam_mat,
		Vec3(0.1, 0.1, 0.1)
	);*/
	
	//Final Pass
	use_default_framebuffer();

	glViewport(0, 0, window_width, window_height);
	set_perspective((double)window_width / window_height);
	glDisable(GL_BLEND);

	switch(mode)
	{
		case NORMAL:
			final_program->use();
			draw_fsq();
			break;

		case COPY_TEXTURES:
			dump_program->use();
			
			dump_program->set_texture("tex", 0, gbuffer_albedo);
			draw_qsq(0);
			dump_program->set_texture("tex", 0, gbuffer_position);
			draw_qsq(1);
			dump_program->set_texture("tex", 0, gbuffer_normal);
			draw_qsq(2);
			dump_program->set_texture("tex", 0, gbuffer_depth);
			draw_qsq(3);
			break;

		case DUMP_LIGHT_MAP:
			glClear(GL_COLOR_BUFFER_BIT);
			dump_cube_program->use();
			dump_cube_program->set_texture("tex", 0, light_map);
			dump_cube_program->set_float("z_mult", 1);
			draw_hsq(0);
			dump_cube_program->set_float("z_mult", -1);
			draw_hsq(1);
			break;

		default:
			dump_program->use();
			switch(mode) 
			{
				case DUMP_ALBEDO:
					dump_program->set_texture("tex", 0, gbuffer_albedo);
					break;
				case DUMP_POSITION:
					dump_program->set_texture("tex", 0, gbuffer_position);
					break;
				case DUMP_NORMAL:
					dump_program->set_texture("tex", 0, gbuffer_normal);
					break;
				case DUMP_DEPTH:
					dump_program->set_texture("tex", 0, gbuffer_depth);
					break;
			}
			draw_fsq();
			break;
	}
	
	glFlush();
	glutSwapBuffers();
	glutPostRedisplay();
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
			fog_scale -= FOG_INCREMENT;
			if(fog_scale < 0.0)
				fog_scale = 0.0;
			break;
		case ']':
			fog_scale += FOG_INCREMENT;
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
			mode = DUMP_ALBEDO;
			break;
		case GLUT_KEY_F4:
			mode = DUMP_POSITION;
			break;
		case GLUT_KEY_F5:
			mode = DUMP_NORMAL;
			break;
		case GLUT_KEY_F6:
			mode = DUMP_DEPTH;
			break;
		case GLUT_KEY_F7:
			mode = DUMP_LIGHT_MAP;
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
