#include "GL/glew.h"
#include "glut.h"
#include "Vector.h"

#include <stdio.h>

GLuint vbo;
GLuint shader_program;


void compile_shader(GLuint shader_object, const GLchar *shader_text)
{
	glShaderSource(shader_object, 1, &shader_text, NULL);
	glCompileShader(shader_object);

	GLint success = 0;
	glGetShaderiv(shader_object, GL_COMPILE_STATUS, &success);
	if(success == GL_FALSE)
	{
		GLint log_length = 0;
		glGetShaderiv(shader_object, GL_INFO_LOG_LENGTH, &log_length);
		GLchar* log = new GLchar[log_length];

		glGetShaderInfoLog(shader_object, log_length, &log_length, log);
		printf(log);

		delete log;
		exit(-1);
	}
}


void init()
{
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	compile_shader(
		vertex_shader,
		"#version 460\n"
		"layout (location = 0) in vec3 position;\n"
		"void main() {\
			gl_Position = vec4(position.x, position.y, position.z, 1);\
		}\n"
	);

	GLuint geometry_shader = glCreateShader(GL_GEOMETRY_SHADER);
	compile_shader(
		geometry_shader,
		"#version 460\n"
		"layout (invocations = 2) in;\n"
		"layout (triangles) in;\n"
		"layout (triangle_strip, max_vertices = 3) out;\n"
		"void main() {\n"
			"gl_Position = gl_in[0].gl_Position + gl_InvocationID * vec4(0.25, -0.5, 0, 0);\n"
			"EmitVertex();\n"
			"gl_Position = gl_in[1].gl_Position + gl_InvocationID * vec4(0.25, -0.5, 0, 0);\n"
			"EmitVertex();\n"
			"gl_Position = gl_in[2].gl_Position + gl_InvocationID * vec4(0.25, -0.5, 0, 0);\n"
			"EmitVertex();\n"
			"EndPrimitive();\n"
		"}\n"
	);

	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	compile_shader(
		fragment_shader,
		"#version 460\n"
		"out vec4 fragColor;\n"
		"void main() {\
			fragColor = vec4(0.5, 0, 0.5, 1);\
		}\n"
	);

	shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, geometry_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);

	GLint status;
	glGetProgramiv(shader_program, GL_LINK_STATUS, &status);
	if(status == GL_FALSE)
	{
		printf("Failed to link program.\n");
		exit(-1);
	}

	//glUseProgram(shader_program);
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

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
