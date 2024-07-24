#pragma once

#include "GL/glew.h"
#include "Vector.h"
#include <vector>
#include <functional>
#include <map>
#include <set>


/*
	Build a ShaderProgram with ShaderProgram::get(vertex_shader, geometry_shader, fragment_shader).
	Build the constituent shaders with Shader::get(shader_core, std::set<const char*> {DEFINE_SOMETHING, DEFINE_SOMETHING_ELSE}).

	The *::get() functions cache the individual object generated, so renderers with the same requirements 
	will share the same programs and programs that need the same shaders will share shaders.

	Shaders are built out of ShaderCores and ShaderOptions. The ShaderCore contains the main chunk of 
	GLSL for the shader, and ShaderOptions correspond to #defines.

	Uniforms are divided into domains:
		long term
		per frame
		per model
	Per model uniform management is done in Model.cpp. Long term and per frame uniforms are managed 
	by ShaderPullFuncs. ShaderPullFuncs set uniforms on the shader program from global vars (in S3.h). 
	Both ShaderCores and ShaderOptions can define ShaderPullFuncs. init funcs should be called 
	whenever the window is resized, via ShaderProgram::init_all(). (They are also called when the shader 
	program is created.) frame funcs should be called every frame after the cam_mat is updated but 
	before anything is drawn, via ShaderProgram::frame_all().
*/


typedef std::function<void(int)> ShaderPullFunc;


#define DEFINE_VERTEX_COLOR			"#define VERTEX_COLOR\n"
#define DEFINE_INSTANCED_XFORM		"#define INSTANCED_XFORM\n"
#define DEFINE_INSTANCED_BASE_COLOR	"#define INSTANCED_BASE_COLOR\n"


struct ShaderOption
{
	ShaderOption(const char* def_name, ShaderPullFunc init_func = NULL, ShaderPullFunc frame_func = NULL)
		: def_name(def_name), init_func(init_func), frame_func(frame_func) {}

	const char* def_name;
	ShaderPullFunc init_func, frame_func;
};


struct ShaderCore
{
	ShaderCore(const char* name, GLuint shader_type, const char* core_text, ShaderPullFunc init_func, ShaderPullFunc frame_func, std::vector<ShaderOption*> options)
		: name(name), shader_type(shader_type), core_text(core_text), init_func(init_func), frame_func(frame_func), options()
	{
		for(auto option : options)
			this->options[option->def_name] = option;
	}

	const char* name;
	GLuint shader_type;
	const char* core_text;
	ShaderPullFunc init_func, frame_func;
	std::map<const char*, ShaderOption*> options;
};


class Shader
{
public:
	GLuint get_id() {return id;}

	void init(GLuint program_id) {init_func(program_id);}
	void frame(GLuint program_id) {frame_func(program_id);}

	ShaderCore* get_core() {return core;}
	const std::set<const char*> get_options() {return options;}

private:
	Shader(ShaderCore* core, const std::set<const char*> options);

	ShaderCore* core;
	const std::set<const char*> options;

	GLuint id;
	ShaderPullFunc init_func, frame_func;

public:
	static Shader* get(ShaderCore* core, const std::set<const char*> options);

private:
	static std::vector<Shader*> all_shaders;
};


extern ShaderCore *vert, *geom_points, *geom_triangles, *frag;

void init_shaders();



class ShaderProgram
{
public:
	GLuint get_id() {return id;}
	void use() {glUseProgram(id);}
	void init() {vertex->init(id); geometry->init(id); fragment->init(id);}
	void frame() {vertex->frame(id); geometry->frame(id); fragment->frame(id);}

	void set_uniform_matrix(const char* name, Mat4& mat);

	Shader* get_vertex() {return vertex;}
	Shader* get_geometry() {return geometry;}
	Shader* get_fragment() {return fragment;}

	void dump() const;

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

private:
	static std::vector<ShaderProgram*> all_shader_programs;
};


void _set_uniform_matrix(GLuint program_id, const char* name, Mat4& mat);
