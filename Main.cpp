#include <stdlib.h>
#include "GL/glew.h"
#include "glut.h"
#include "Vector.h"
#include "Shaders.h"
#include "Utils.h"
#include "Model.h"
#include "S3.h"

#include <stdio.h>
#include <time.h>


Model* dots_model = NULL;
Model* pole_model = NULL;

#define NUM_DOTS		(2000)

#define TRANSLATION_SPEED		(TAU / 12)
#define ROTATION_SPEED			(TAU / 6)

#define FOG_INCREMENT		(0.5)

struct Controls
{
	bool	fwd, back,
			right, left,
			up, down,

			pitch_up, pitch_down,
			yaw_right, yaw_left,
			roll_right, roll_left;
};
Controls controls;

double last_fame_time;


void init()
{
	srand(clock());

	glClearColor(0, 0, 0, 0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_PROGRAM_POINT_SIZE);

	init_shaders();
	init_models();

	Vec4* dots = new Vec4[NUM_DOTS];
	for(int i = 0; i < NUM_DOTS; i++)
	{
		do {
			for(int j = 0; j < 4; j++)
				dots[i].components[j] = fsrand();
		} while(dots[i].mag2() == 0 || dots[i].mag2() > 1);
		dots[i].normalize_in_place();
	}
	dots_model = new Model(NUM_DOTS, dots);

	pole_model = Model::read_model_file("subdivided_icosahedron.model", 0.05);
}


#define NEAR		(0.001)
#define FAR			(TAU)
#define HALF_FOV	(TAU / 8)

void reshape(int w, int h)
{
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	
	aspect_ratio = (double)w / h;
	double q = tan(HALF_FOV);

	proj_mat = Mat4(
		1.0 / (aspect_ratio * q),	0,					0,								0,
		0,							1.0 / q,			0,								0,
		0,							0,					(FAR + NEAR) / (NEAR - FAR),	2.0 * NEAR * FAR / (NEAR - FAR),
		0,							0,					-1,								0
	);
}

void display()
{
	double dt = current_time() - last_fame_time;
	printf("%f\n", 1.0 / dt);
	print_matrix(cam_mat);
	printf("\n");

	#define CONTROL_SPEED(positive, negative, speed)	(\
															(positive && !negative) ? speed * dt : \
																(negative && !positive) ? -speed * dt : 0\
														)
	translate_cam(
		CONTROL_SPEED(controls.right, controls.left, TRANSLATION_SPEED),
		CONTROL_SPEED(controls.up, controls.down, TRANSLATION_SPEED),
		CONTROL_SPEED(controls.fwd, controls.back, TRANSLATION_SPEED)
	);
	rotate_cam(
		CONTROL_SPEED(controls.pitch_up, controls.pitch_down, ROTATION_SPEED),
		CONTROL_SPEED(controls.yaw_right, controls.yaw_left, ROTATION_SPEED),
		CONTROL_SPEED(controls.roll_right, controls.roll_left, ROTATION_SPEED)
	);

	last_fame_time += dt;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	pole_model->draw(Mat4::identity(), Vec4(1, 1, 1, 1));
	pole_model->draw(Mat4::axial_rotation(_w, _x, TAU / 4), Vec4(1, 0, 0, 1));
	pole_model->draw(Mat4::axial_rotation(_w, _y, TAU / 4), Vec4(0, 1, 0, 1));
	pole_model->draw(Mat4::axial_rotation(_w, _z, TAU / 4), Vec4(0, 0, 1, 1));
	
	dots_model->draw(Mat4::identity(), Vec4(1, 1, 1, 1));

	geodesic_model->draw(Mat4::identity(), Vec4(0, 0.5, 1, 1));
	geodesic_model->draw(Mat4::axial_rotation(_x, _z, TAU / 4) * Mat4::axial_rotation(_y, _w, TAU / 4), Vec4(1, 0.5, 0, 1));

	glFlush();
	glutSwapBuffers();
	glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y)
{
	switch(key)
	{
		case 27:
			exit(0);
			break;
		
		case '2':
			controls.pitch_up = true;
			break;
		case '8':
			controls.pitch_down = true;
			break;

		case '6':
			controls.yaw_right = true;
			break;
		case '4':
			controls.yaw_left = true;
			break;

		case '3':
			controls.roll_right = true;
			break;
		case '1':
			controls.roll_left = true;
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
		case '2':
			controls.pitch_up = false;
			break;
		case '8':
			controls.pitch_down = false;
			break;

		case '6':
			controls.yaw_right = false;
			break;
		case '4':
			controls.yaw_left = false;
			break;

		case '3':
			controls.roll_right = false;
			break;
		case '1':
			controls.roll_left = false;
			break;
	}
}

void special(int key, int x, int y)
{
	switch(key)
	{
		/*case GLUT_KEY_F1:
			draw_poles = !draw_poles;
			break;
		case GLUT_KEY_F2:
			draw_clutter = !draw_clutter;
			break;
		case GLUT_KEY_F3:
			draw_sun_paths = !draw_sun_paths;
			break;
		case GLUT_KEY_F7:
			draw_hopf = !draw_hopf;
			break;
		case GLUT_KEY_F8:
			draw_antihopf = !draw_antihopf;
			break;*/

		case GLUT_KEY_UP:
			controls.fwd = true;
			break;
		case GLUT_KEY_DOWN:
			controls.back = true;
			break;

		case GLUT_KEY_RIGHT:
			controls.right = true;
			break;
		case GLUT_KEY_LEFT:
			controls.left = true;
			break;

		case GLUT_KEY_PAGE_UP:
			controls.up = true;
			break;
		case GLUT_KEY_PAGE_DOWN:
			controls.down = true;
			break;
	}
}

void special_up(int key, int x, int y)
{
	switch(key)
	{
		case GLUT_KEY_UP:
			controls.fwd = false;
			break;
		case GLUT_KEY_DOWN:
			controls.back = false;
			break;

		case GLUT_KEY_RIGHT:
			controls.right = false;
			break;
		case GLUT_KEY_LEFT:
			controls.left = false;
			break;

		case GLUT_KEY_PAGE_UP:
			controls.up = false;
			break;
		case GLUT_KEY_PAGE_DOWN:
			controls.down = false;
			break;
	}
}


int main(int argc, char **argv)
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
	glutSpecialFunc(special);
	glutSpecialUpFunc(special_up);

	init();

	glutMainLoop();
	return 0;
}
