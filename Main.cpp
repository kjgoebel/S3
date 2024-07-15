#include "GL/glew.h"
#include "glut.h"
#include "Vector.h"
#include "Shaders.h"

#include <stdio.h>

GLuint vbo;
GLuint shader_program;


void init()
{
	init_shaders();
	shader_program = make_new_program(vert, geom_triangles, frag_simple);

	Vec4 vertices[] = {
		Vec4(-0.5, -0.5),
		Vec4(0.5, -0.5),
		Vec4(0, 0.75)
	};
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 4, GL_DOUBLE, GL_FALSE, 4 * sizeof(double), (void*)0);
	glEnableVertexAttribArray(0);
}

void reshape(int w, int h)
{
	
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shader_program);
	glBindVertexArray(vbo);
	glDrawArrays(GL_TRIANGLES, 0, 3);

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
