#include <stdlib.h>
#include "GL/glew.h"
#include "GL/glut.h"

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
					* Mat4::axial_rotation(_y, _w, 0.001)		//head position above feet
					* Mat4::axial_rotation(_z, _x, yaw)			//yaw
					* Mat4::axial_rotation(_y, _z, pitch);		//pitch
	}
};
PlayerState player_state;

#define WALK_SPEED		(TAU / 50)

#define FOG_INCREMENT		(0.5)

GLuint gbuffer = 0, color_tex = 0, depth_buffer = 0;

Vec4 fsq_vertices[4] = {
	{1, 1, 0, 1},
	{-1, 1, 0, 1},
	{-1, -1, 0, 1},
	{1, -1, 0, 1}
};
GLuint fsq_vertex_array;

ShaderProgram* fsq_program = NULL;


void init()
{
	srand(clock());

	init_shaders();

	fsq_program = ShaderProgram::get(
		Shader::get(vert_screenspace, {}),
		NULL,
		Shader::get(frag_copy_texture, {})
	);

	glGenVertexArrays(1, &fsq_vertex_array);
	glBindVertexArray(fsq_vertex_array);

	GLuint fsq_vertex_buffer;
	glGenBuffers(1, &fsq_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, fsq_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(fsq_vertices), fsq_vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 4, GL_DOUBLE, GL_FALSE, sizeof(Vec4), (void*)0);
	glEnableVertexAttribArray(0);

	glClearColor(0, 0, 0, 0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	torus_model = Model::make_torus(128, 128, 1, false);
	torus_model->generate_primitive_colors(0.7);

	player_state.a = player_state.b = player_state.yaw = player_state.pitch = 0;
	player_state.set_cam();
}

int window_width, window_height;

void reshape(int w, int h)
{
	window_width = w;
	window_height = h;

	ShaderProgram::init_all();

	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	set_perspective((double)w / h);

	if(gbuffer)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);
		glBindTexture(GL_TEXTURE_2D, color_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, NULL);
		glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
	}
	else
	{
		glGenFramebuffers(1, &gbuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);

		glGenTextures(1, &color_tex);
		glBindTexture(GL_TEXTURE_2D, color_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glNamedFramebufferTexture(gbuffer, GL_COLOR_ATTACHMENT0, color_tex, 0);

		glGenRenderbuffers(1, &depth_buffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
	}

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		error("Framebuffer status is %d\n", glCheckFramebufferStatus(GL_FRAMEBUFFER));
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

	glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);

	glClearColor(0, 0, 0, 0);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ShaderProgram::frame_all();

	torus_model->draw(Mat4::identity(), Vec4(0.3, 0.3, 0.3, 1));

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	glDisable(GL_DEPTH_TEST);

	glDisable(GL_CULL_FACE);

	glClearColor(0, 0.5, 0, 0);

	fsq_program->use();
	GLuint color_location = glGetUniformLocation(fsq_program->get_id(), "color_tex");
	glUniform1i(color_location, 0);
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, color_tex);
	
	glBindVertexArray(fsq_vertex_array);
	glDrawArrays(GL_QUADS, 0, 4);

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
