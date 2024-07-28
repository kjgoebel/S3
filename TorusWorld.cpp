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


Model* torus_model;
Model* pole_model;
ShaderProgram* fsq_program = NULL;

double last_frame_time;

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
		double ca = cos(a), sa = sin(a), cb = cos(b), sb = sin(b);
		Vec4 pos = INV_ROOT_2 * Vec4(cb, sb, ca, sa);
		Vec4 fwd = Vec4(0, 0, -sa, ca);

		Vec4 down = INV_ROOT_2 * Vec4(-cb, -sb, ca, sa);
		Vec4 right = Vec4(sb, -cb, 0, 0);

		cam_mat = Mat4::from_columns(right, down, fwd, pos)		//location on the torus
					* Mat4::axial_rotation(_y, _w, 0.001)		//head position above feet
					* Mat4::axial_rotation(_z, _x, yaw)			//yaw
					* Mat4::axial_rotation(_y, _z, pitch);		//pitch
	}
};
PlayerState player_state;

#define WALK_SPEED		(TAU / 50)

#define FOG_INCREMENT		(0.5)


void init()
{
	srand(clock());

	glCullFace(GL_BACK);

	init_shaders();

	fsq_program = ShaderProgram::get(
		Shader::get(vert_screenspace, {}),
		NULL,
		Shader::get(frag_point_light, {})
	);

	torus_model = Model::make_torus(128, 128, 1, false);
	torus_model->generate_primitive_colors(0.7);

	pole_model =  new Model(
		GL_TRIANGLES,
		NUM_POLE_VERTS,
		3,
		NUM_POLE_TRIANGLES,
		Model::s3ify(NUM_POLE_VERTS, 0.05, pole_model_vertices).get(),
		pole_model_elements
	);
	pole_model->generate_primitive_colors(0.3);

	player_state.a = player_state.b = player_state.yaw = player_state.pitch = 0;
	player_state.set_cam();
}

int window_width, window_height;

void reshape(int w, int h)
{
	window_width = w;
	window_height = h;

	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	set_perspective((double)w / h);

	init_framebuffer(w, h);

	ShaderProgram::init_all();
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

	use_gbuffer();

	glDisable(GL_BLEND);

	ShaderProgram::frame_all();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	torus_model->draw(Mat4::identity(), Vec4(0.3, 0.3, 0.3, 1));
	pole_model->draw(Mat4::axial_rotation(_w, _x, TAU / 4), Vec4(0.7, 0, 0, 1));
	pole_model->draw(Mat4::axial_rotation(_w, _y, TAU / 4), Vec4(0, 0.7, 0, 1));

	use_default_framebuffer();

	glClear(GL_COLOR_BUFFER_BIT);
	fsq_program->use();

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	
	GLuint program_id = fsq_program->get_id();

	Vec4 light_pos = ~cam_mat * Vec4(0, 1, 0, 0);
	Vec4 light_emission = Vec3(0.3, 0.3, 0.15);
	glProgramUniform4f(program_id, glGetUniformLocation(program_id, "light_pos"), light_pos.x, light_pos.y, light_pos.z, light_pos.w);
	glProgramUniform3f(program_id, glGetUniformLocation(program_id, "light_emission"), light_emission.x, light_emission.y, light_emission.z);
	draw_fsq();

	light_pos = ~cam_mat * Vec4(1, 0, 0, 1).normalize();
	light_emission = Vec3(0.1, 0.1, 0.2);
	glProgramUniform4f(program_id, glGetUniformLocation(program_id, "light_pos"), light_pos.x, light_pos.y, light_pos.z, light_pos.w);
	glProgramUniform3f(program_id, glGetUniformLocation(program_id, "light_emission"), light_emission.x, light_emission.y, light_emission.z);
	draw_fsq();
	
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
	glutPassiveMotionFunc(mouse);
	glutMotionFunc(mouse);

	init();

	glutSetCursor(GLUT_CURSOR_NONE);

	glutMainLoop();
	return 0;
}
