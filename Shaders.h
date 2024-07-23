#pragma once

#include "GL/glew.h"
#include "Vector.h"
#include <vector>
#include <functional>


typedef std::function<void(int)> ShaderPullFunc;


class Shader
{
public:
	//Note that init() will have to be called whenever the window is resized. There's a case to be made for much greater specificity in these funcs.
	Shader(GLuint shader_type, const std::vector<char*> text, ShaderPullFunc init_func = NULL, ShaderPullFunc frame_func = NULL);

	GLuint get_id() {return id;}

	void init(GLuint program_id) {if(init_func) init_func(program_id);}
	void frame(GLuint program_id) {if(frame_func) frame_func(program_id);}

private:
	GLuint id;

	ShaderPullFunc init_func, frame_func;
};


extern Shader *vert, *geom_points, *geom_triangles, *frag_simple, *frag_vcolor;

void init_shaders();



class ShaderProgram
{
public:
	GLuint get_id() {return id;}
	void use() {glUseProgram(id);}
	void init() {vertex->init(id); geometry->init(id); fragment->init(id);}
	void frame() {vertex->frame(id); geometry->frame(id); fragment->frame(id);}

	void set_uniform_matrix(const char* name, Mat4& mat);

private:
	ShaderProgram(Shader* vert, Shader* geom, Shader* frag);

	GLuint id;

	Shader* vertex;
	Shader* geometry;
	Shader* fragment;

public:
	static ShaderProgram* get(Shader* vert, Shader* geom, Shader* frag);

	static void init_all();
	static void frame_all();

	static std::vector<ShaderProgram*> all_shader_programs;
};


void _set_uniform_matrix(GLuint program_id, const char* name, Mat4& mat);
