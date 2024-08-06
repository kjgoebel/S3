#include <stdlib.h>
#include "GL/glew.h"
#include "gl/glut.h"
#include "Vector.h"
#include "Shaders.h"
#include "Utils.h"
#include "Model.h"
#include "Camera.h"
#include "Framebuffer.h"

#include <stdio.h>
#include <time.h>


#define PRINT_FRAME_RATE

#define NUM_DOTS		(2000)

#define TRANSLATION_SPEED		(TAU / 16)
#define ROTATION_SPEED			(TAU / 6)

#define FOG_INCREMENT		(0.5)

Pass* gpass = NULL;
Pass* fog_pass = NULL;

ShaderProgram* fog_quad_program = NULL;

Model* dots_model = NULL;
Model* pole_model = NULL;
Model* geodesic_model = NULL;
Model* torus_model = NULL;
Model* tesseract_arc = NULL;

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
		draw_cross = false,			//the 16-cell {3, 3, 4}, projected onto the Sphere
		draw_torus = false,			//the Clifford torus defined by the selected Hopf and "anti-Hopf" fibers
		draw_superhopf = false,		//a lot of fibers from a Hopf bundle
		draw_tesseract = false,		//the tesseract {4, 3, 3}, projected onto the Sphere
		draw_itc = false,			//the regular icositetrachoron {3, 4, 3}, projected onto the Sphere
		draw_dual_itc = false;		//the icositetrachoron dual to the one above

#define STANDARD_HOLE_RATIO		(0.006)

DrawFunc render_poles = NULL;

#define NUM_HOPF_FIBERS		(32)
DrawFunc render_hopf = NULL, render_antihopf = NULL;

#define TORUS_SUBDIVISIONS NUM_HOPF_FIBERS

#define NUM_SUPERHOPF_FIBERS		(1024)
DrawFunc render_superhopf = NULL;

#define NUM_TESSERACT_EDGES			(32)
DrawFunc render_tesseract = NULL;

#define NUM_CROSS_EDGE_LOOPS		(8)
DrawFunc render_cross = NULL;

#define NUM_ITC_EDGE_LOOPS			(16)
DrawFunc render_itc = NULL, render_dual_itc = NULL;


void init()
{
	glClearColor(0, 0, 0, 0);
	//glEnable(GL_PROGRAM_POINT_SIZE);

	init_random();
	init_luts();
	init_shaders();
	init_framebuffers();

	gpass = new Pass(s_gbuffer);
	fog_pass = new Pass(NULL);
	fog_pass->clear_mask = 0;
	fog_pass->depth_test = false;
	fog_pass->depth_mask = false;

	fog_quad_program = ShaderProgram::get(
		Shader::get(vert_screenspace, {}),
		NULL,
		Shader::get(frag_fog, {})
	);

	int i;

	Vec4* dots = new Vec4[NUM_DOTS];
	for(i = 0; i < NUM_DOTS; i++)
		dots[i] = rand_s3();
	dots_model = new Model(NUM_DOTS, dots);
	delete[] dots;

	pole_model = Model::make_icosahedron(0.05, 2);
	pole_model->generate_primitive_colors(0.3);
	geodesic_model = Model::make_torus(32, 8, STANDARD_HOLE_RATIO);
	geodesic_model->generate_primitive_colors(0.5);
	torus_model = Model::make_torus(NUM_HOPF_FIBERS, NUM_HOPF_FIBERS, 1, false);
	torus_model->generate_primitive_colors(0.7);

	Mat4 pole_xforms[4] = {
		Mat4::identity(),
		Mat4::axial_rotation(_w, _x, TAU / 4),
		Mat4::axial_rotation(_w, _y, TAU / 4),
		Mat4::axial_rotation(_w, _z, TAU / 4)
	};
	Vec4 pole_colors[4] = {
		{0.7, 0.7, 0.7, 1},
		{0.7, 0, 0, 1},
		{0, 0.7, 0, 1},
		{0, 0, 0.7, 1}
	};
	render_poles = pole_model->make_draw_func(4, pole_xforms, pole_colors);

	Mat4 hopf_xforms[NUM_HOPF_FIBERS];
	Mat4 antihopf_xforms[NUM_HOPF_FIBERS];
	for(i = 0; i < NUM_HOPF_FIBERS; i++)
	{
		double theta = (double)i * TAU / (2 * NUM_HOPF_FIBERS);
		hopf_xforms[i] = Mat4::axial_rotation(_x, _w, theta) * Mat4::axial_rotation(_y, _z, theta);
		antihopf_xforms[i] = Mat4::axial_rotation(_y, _x, theta) * Mat4::axial_rotation(_z, _w, theta) * Mat4::axial_rotation(_x, _z, TAU / 4);
	}
	render_hopf = geodesic_model->make_draw_func(NUM_HOPF_FIBERS, hopf_xforms, Vec4(1, 0.5, 0, 1));
	render_antihopf = geodesic_model->make_draw_func(NUM_HOPF_FIBERS, antihopf_xforms, Vec4(0, 1, 0.5, 1));

	Mat4 superhopf_xforms[NUM_SUPERHOPF_FIBERS];
	for(i = 0; i < NUM_SUPERHOPF_FIBERS; i++)
	{
		Vec3 temp = rand_s2();
		double theta = 0.5 * acos(temp.z), phi = atan2(temp.y, temp.x);

		superhopf_xforms[i] =
			Mat4::axial_rotation(_x, _y, phi)
			* Mat4::axial_rotation(_w, _x, theta)
			* Mat4::axial_rotation(_y, _z, theta);
			//* Mat4::axial_rotation(_w, _z, frand() * TAU);		//Random longitudinal displacement so that the stripes on nearby fibers don't line up. It might be more elucidating if they do line up, come to think of it.
	}
	render_superhopf = geodesic_model->make_draw_func(NUM_SUPERHOPF_FIBERS, superhopf_xforms, Vec4(0.5, 1, 0.5, 1));

	Mat4 tesseract_edge_xforms[NUM_TESSERACT_EDGES];
	tesseract_arc = Model::make_torus_arc(8, 8, acos(0.5), STANDARD_HOLE_RATIO);
	tesseract_arc->generate_primitive_colors(0.5);
	int edge_index = 0;
	for(int vertex = 0; vertex < 16; vertex++)
	{
		Vec4 a = Vec4(
			vertex & 1 ? 1 : -1,
			vertex & 2 ? 1 : -1,
			vertex & 4 ? 1 : -1,
			vertex & 8 ? 1 : -1
		).normalize();
		for(int axis = 0; axis < 4; axis++)
		{
			if(vertex & (1 << axis))
			{
				int other_vertex = vertex & ~(1 << axis);
				Vec4 b = Vec4(
					other_vertex & 1 ? 1 : -1,
					other_vertex & 2 ? 1 : -1,
					other_vertex & 4 ? 1 : -1,
					other_vertex & 8 ? 1 : -1
				).normalize();

				tesseract_edge_xforms[edge_index] = basis_around(a, b);
				edge_index++;
			}
		}
	}
	render_tesseract = tesseract_arc->make_draw_func(NUM_TESSERACT_EDGES, tesseract_edge_xforms, Vec4(1, 0, 1, 1));

	Mat4 cross_xforms[NUM_CROSS_EDGE_LOOPS] = {
		Mat4::identity(),
		Mat4::axial_rotation(_x, _w, TAU / 4),
		Mat4::axial_rotation(_y, _w, TAU / 4),
		Mat4::axial_rotation(_x, _z, TAU / 4),
		Mat4::axial_rotation(_y, _z, TAU / 4),
		Mat4::axial_rotation(_x, _w, TAU / 4) * Mat4::axial_rotation(_y, _z, TAU / 4)
	};
	render_cross = geodesic_model->make_draw_func(NUM_CROSS_EDGE_LOOPS, cross_xforms, Vec4(1, 0, 0, 1));

	//Don't ask how I came up with these vectors. You don't want to know.
	Mat4 itc_edge_loop_xforms[NUM_ITC_EDGE_LOOPS];
	Vec4 temp[NUM_ITC_EDGE_LOOPS][2] = {
		{Vec4(0, 1, 1, 0), Vec4(1, 0, 1, 0)},
		{Vec4(0, 1, 1, 0), Vec4(1, 1, 0, 0)},
		{Vec4(0, 1, 1, 0), Vec4(0, 0, 1, 1)},
		{Vec4(0, 1, 1, 0), Vec4(0, 0, 1, -1)},
		{Vec4(1, 1, 0, 0), Vec4(0, 1, -1, 0)},
		{Vec4(1, 1, 0, 0), Vec4(0, 1, 0, 1)},
		{Vec4(1, 1, 0, 0), Vec4(0, 1, 0, -1)},
		{Vec4(-1, 0, 0, 1), Vec4(0, -1, 0, 1)},
		{Vec4(-1, 0, 0, 1), Vec4(0, 0, 1, 1)},
		{Vec4(-1, 0, 0, 1), Vec4(0, 0, -1, 1)},
		{Vec4(0, 0, 1, -1), Vec4(1, 0, 1, 0)},
		{Vec4(0, 0, 1, -1), Vec4(0, 1, 0, -1)},
		{Vec4(0, 1, 0, 1), Vec4(1, 0, 0, 1)},
		{Vec4(0, 1, 0, 1), Vec4(0, 0, 1, 1)},
		{Vec4(0, -1, 1, 0), Vec4(1, -1, 0, 0)},
		{Vec4(1, 0, 0, 1), Vec4(0, 0, 1, 1)}
	};
	for(i = 0; i < NUM_ITC_EDGE_LOOPS; i++)
		itc_edge_loop_xforms[i] = basis_around(temp[i][0].normalize(), temp[i][1].normalize());

	render_itc = geodesic_model->make_draw_func(NUM_ITC_EDGE_LOOPS, itc_edge_loop_xforms, Vec4(1, 0, 1, 1));

	Mat4 dual_rotation = Mat4::axial_rotation(_w, _z, TAU / 8) * Mat4::axial_rotation(_y, _x, TAU / 8);
	for(i = 0; i < NUM_ITC_EDGE_LOOPS; i++)
		itc_edge_loop_xforms[i] = dual_rotation * itc_edge_loop_xforms[i];
	render_dual_itc = geodesic_model->make_draw_func(NUM_ITC_EDGE_LOOPS, itc_edge_loop_xforms, Vec4(1, 0, 0, 1));
}


void reshape(int w, int h)
{
	cam.set_perspective((double)w / h);
	resize_screenbuffers(w, h);
	ShaderProgram::init_all();
}

void display()
{
	double dt = current_time() - last_fame_time;

	#ifdef PRINT_FRAME_RATE
		printf("%f\n", 1.0 / dt);
		print_matrix(cam.get_mat());
		printf("\n");
	#endif

	#define CONTROL_SPEED(positive, negative, speed)	(\
		(positive && !negative) ? speed * dt : \
			(negative && !positive) ? -speed * dt : 0\
	)
	cam.translate(
		CONTROL_SPEED(controls.right, controls.left, TRANSLATION_SPEED),
		CONTROL_SPEED(controls.down, controls.up, TRANSLATION_SPEED),
		CONTROL_SPEED(controls.fwd, controls.back, TRANSLATION_SPEED)
	);
	cam.rotate(
		CONTROL_SPEED(controls.pitch_up, controls.pitch_down, ROTATION_SPEED),
		CONTROL_SPEED(controls.yaw_right, controls.yaw_left, ROTATION_SPEED),
		CONTROL_SPEED(controls.roll_right, controls.roll_left, ROTATION_SPEED)
	);

	last_fame_time += dt;

	gpass->start();

	ShaderProgram::frame_all();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if(draw_poles)
		render_poles();

	if(draw_clutter)
		dots_model->draw(Mat4::identity(), Vec4(1, 1, 1, 1));

	if(draw_tesseract)
		render_tesseract();

	if(draw_cross)
		render_cross();

	if(draw_itc)
	{
		render_itc();
		if(draw_dual_itc)
			render_dual_itc();
	}

	if(draw_hopf)
	{
		render_hopf();
		if(draw_antihopf)
			render_antihopf();
	}

	if(draw_sun_paths)
	{
		geodesic_model->draw(Mat4::axial_rotation(_y, _w, TAU / 8) * Mat4::axial_rotation(_z, _x, TAU / 8), Vec4(1, 0, 0, 1));
		geodesic_model->draw(Mat4::axial_rotation(_x, _z, TAU / 8) * Mat4::axial_rotation(_w, _y, TAU / 8), Vec4(0, 0, 1, 1));
	}
	
	if(draw_torus)
		torus_model->draw(Mat4::axial_rotation(_y, _w, TAU / 8) * Mat4::axial_rotation(_z, _x, TAU / 8), Vec4(0.3, 0.3, 0.3, 1));

	if(draw_superhopf)
		render_superhopf();

	fog_pass->start();
	fog_quad_program->use();
	draw_fsq();

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
		case GLUT_KEY_F4:
			draw_tesseract = !draw_tesseract;
			break;
		case GLUT_KEY_F5:
			draw_itc = !draw_itc;
			break;
		case GLUT_KEY_F6:
			draw_dual_itc = !draw_dual_itc;
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
		case GLUT_KEY_F11:
			draw_superhopf = !draw_superhopf;
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
