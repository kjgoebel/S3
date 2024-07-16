#include <stdlib.h>
#include "GL/glew.h"
#include "glut.h"
#include "Vector.h"
#include "Shaders.h"
#include "Utils.h"
#include "Model.h"

#include <stdio.h>
#include <time.h>

GLuint vbo, vao;
GLuint shader_program;

Model *dots_model = NULL;

#define NUM_DOTS		(100)


void init()
{
	srand(clock());

	init_shaders();
	shader_program = make_new_program(vert, geom_triangles, frag_simple);

	Vec4 vertices[] = {
		Vec4(-0.5, -0.5),
		Vec4(0.5, -0.5),
		Vec4(0, 0.75)
	};
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 4, GL_DOUBLE, GL_FALSE, 4 * sizeof(double), (void*)0);
	glEnableVertexAttribArray(0);

	Vec4* dots = new Vec4[NUM_DOTS];
	for(int i = 0; i < NUM_DOTS; i++)
		for(int j = 0; j < 4; j++)
			dots[i].components[j] = fsrand();
	dots_model = new Model(GL_POINTS, NUM_DOTS, NUM_DOTS, dots);
}

void reshape(int w, int h)
{
	
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shader_program);
	glProgramUniform4f(shader_program, glGetUniformLocation(shader_program, "baseColor"), 0, 0.5, 1, 1);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);		//This apparently generates an "invalid operation" error.

	dots_model->draw(Vec4(1, 0, 0, 1));

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

	glClearColor(0, 0, 0, 0);

	init();

	glutMainLoop();
	return 0;
}
