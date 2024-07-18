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
Model* geodesic_model = NULL;
Model* torus_model = NULL;

#define NUM_DOTS		(2000)

#define TRANSLATION_SPEED		(TAU / 16)
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

bool	draw_poles = true,			//colored markers at each of the +x, +y, +z, +w poles of the sphere
		draw_clutter = true,		//white dots randomly distributed through the space
		draw_sun_paths = false,		//the great circles at the centers of the Clifford torus defined by the selected Hopf and "anti-Hopf" fibers
		draw_hopf = false,			//selected fibers of a Hopf bundle, all passing through a selected great cirle
		draw_antihopf = false,		//fibers of a different Hopf bundle, selected to be always orthogonal to the other set of fibers
		draw_cross = false,			//the 16-cell {3, 3, 4}, projected onto the sphere
		draw_torus = false;			//the Clifford torus defined by the selected Hopf and "anti-Hopf" fibers


#define NUM_HOPF_FIBERS		(32)
Mat4 hopf_xforms[NUM_HOPF_FIBERS];
Mat4 antihopf_xforms[NUM_HOPF_FIBERS];

#define TORUS_SUBDIVISIONS NUM_HOPF_FIBERS

void init()
{
	srand(clock());

	glClearColor(0, 0, 0, 0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_PROGRAM_POINT_SIZE);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	init_shaders();

	int i;

	Vec4* dots = new Vec4[NUM_DOTS];
	for(i = 0; i < NUM_DOTS; i++)
	{
		do {
			for(int j = 0; j < 4; j++)
				dots[i].components[j] = fsrand();
		} while(dots[i].mag2() == 0 || dots[i].mag2() > 1);
		dots[i].normalize_in_place();
	}
	dots_model = new Model(NUM_DOTS, dots);
	delete dots;

	pole_model = Model::read_model_file("subdivided_icosahedron.model", 0.05);
	pole_model->generate_primitive_colors(0.3);

	for(i = 0; i < NUM_HOPF_FIBERS; i++)
	{
		double theta = (double)i * TAU / (2 * NUM_HOPF_FIBERS);
		hopf_xforms[i] = Mat4::axial_rotation(_x, _w, theta) * Mat4::axial_rotation(_y, _z, theta);
		antihopf_xforms[i] = Mat4::axial_rotation(_y, _x, theta) * Mat4::axial_rotation(_z, _w, theta) * Mat4::axial_rotation(_x, _z, TAU / 4);
	}

	geodesic_model = Model::make_torus(128, 8, 0.004);
	geodesic_model->generate_primitive_colors(0.5);
	torus_model = Model::make_torus(NUM_HOPF_FIBERS, NUM_HOPF_FIBERS, 1, false);
	torus_model->generate_primitive_colors(0.7);
}


void reshape(int w, int h)
{
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	set_perspective((double)w / h);
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

	if(draw_poles)
	{
		pole_model->draw(Mat4::identity(), Vec4(0.7, 0.7, 0.7, 1));
		pole_model->draw(Mat4::axial_rotation(_w, _x, TAU / 4), Vec4(0.7, 0, 0, 1));
		pole_model->draw(Mat4::axial_rotation(_w, _y, TAU / 4), Vec4(0, 0.7, 0, 1));
		pole_model->draw(Mat4::axial_rotation(_w, _z, TAU / 4), Vec4(0, 0, 0.7, 1));
	}
	
	if(draw_clutter)
		dots_model->draw(Mat4::identity(), Vec4(1, 1, 1, 1));

	if(draw_hopf)
	{
		for(int i = 0; i < NUM_HOPF_FIBERS; i++)
		{
			geodesic_model->draw(hopf_xforms[i], Vec4(1, 0.5, 0, 1));
			if(draw_antihopf)
				geodesic_model->draw(antihopf_xforms[i], Vec4(0, 1, 0.5, 1));
		}
	}

	if(draw_sun_paths)
	{
		geodesic_model->draw(Mat4::axial_rotation(_y, _w, TAU / 8) * Mat4::axial_rotation(_z, _x, TAU / 8), Vec4(1, 0, 0, 1));
		geodesic_model->draw(Mat4::axial_rotation(_x, _z, TAU / 8) * Mat4::axial_rotation(_w, _y, TAU / 8), Vec4(0, 0, 1, 1));
	}

	if(draw_cross)
	{
		geodesic_model->draw(Mat4::identity(), Vec4(1, 0, 1, 1));
		geodesic_model->draw(Mat4::axial_rotation(_x, _w, TAU / 4), Vec4(1, 0, 1, 1));
		geodesic_model->draw(Mat4::axial_rotation(_y, _w, TAU / 4), Vec4(1, 0, 1, 1));
		geodesic_model->draw(Mat4::axial_rotation(_x, _z, TAU / 4), Vec4(1, 0, 1, 1));
		geodesic_model->draw(Mat4::axial_rotation(_y, _z, TAU / 4), Vec4(1, 0, 1, 1));
		geodesic_model->draw(Mat4::axial_rotation(_x, _w, TAU / 4) * Mat4::axial_rotation(_y, _z, TAU / 4), Vec4(1, 0, 1, 1));
	}
	
	if(draw_torus)
		torus_model->draw(Mat4::axial_rotation(_y, _w, TAU / 8) * Mat4::axial_rotation(_z, _x, TAU / 8), Vec4(0.3, 0.3, 0.3, 1));

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
		case GLUT_KEY_F1:
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
			break;
		case GLUT_KEY_F9:
			draw_cross = !draw_cross;
			break;
		case GLUT_KEY_F10:
			draw_torus = !draw_torus;
			break;

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
