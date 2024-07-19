#include <stdlib.h>
#include "GL/glew.h"
#include "glut.h"

#include "Model.h"
#include "Shaders.h"
#include "Utils.h"

#include <stdio.h>
#include <time.h>


Model* torus_model;

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
					* Mat4::axial_rotation(_y, _w, 0.002)		//head position above feet
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

	glClearColor(0, 0, 0, 0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	init_shaders();

	torus_model = Model::make_torus(128, 128, 1, false);
	torus_model->generate_primitive_colors(0.7);

	player_state.a = player_state.b = player_state.yaw = player_state.pitch = 0;
	//cam_mat = Mat4::axial_rotation(_w, _y, TAU / 8);
	player_state.set_cam();
}

int window_width, window_height;

void reshape(int w, int h)
{
	window_width = w;
	window_height = h;
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	set_perspective((double)w / h);
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

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	torus_model->draw(Mat4::identity(), Vec4(0.3, 0.3, 0.3, 1));

	printf("%f\n", 1.0 / dt);
	print_matrix(cam_mat);
	printf("\n");

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
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

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
