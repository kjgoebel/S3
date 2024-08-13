#pragma once

#include "GL/glew.h"
#include "Vector.h"
#include "Framebuffer.h"
#include <vector>
#include <functional>
#include <map>
#include <set>
#include "LookupTable.h"


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


typedef std::function<void(class ShaderProgram*)> ShaderPullFunc;


#define DEFINE_VERTEX_COLOR			"#define VERTEX_COLOR\n"
#define DEFINE_INSTANCED_XFORM		"#define INSTANCED_XFORM\n"
#define DEFINE_INSTANCED_BASE_COLOR	"#define INSTANCED_BASE_COLOR\n"
#define DEFINE_VERTEX_NORMAL		"#define VERTEX_NORMAL\n"
#define DEFINE_SHADOW				"#define SHADOW\n"
#define DEFINE_HORIZONTAL			"#define HORIZONTAL\n"


struct ShaderOption
{
	ShaderOption(const char* def_name, ShaderPullFunc init_func = NULL, ShaderPullFunc frame_func = NULL, ShaderPullFunc use_func = NULL)
		: def_name(def_name), init_func(init_func), frame_func(frame_func), use_func(use_func) {}

	const char* def_name;
	ShaderPullFunc init_func, frame_func, use_func;
};


struct ShaderCore
{
	ShaderCore(
		const char* name,
		GLuint shader_type,
		const char* core_text,
		ShaderPullFunc init_func,
		ShaderPullFunc frame_func,
		ShaderPullFunc use_func,
		std::vector<ShaderOption*> options
	)
		: name(name), shader_type(shader_type), core_text(core_text), init_func(init_func), frame_func(frame_func), use_func(use_func), options()
	{
		for(auto option : options)
			this->options[option->def_name] = option;
	}

	const char* name;
	GLuint shader_type;
	const char* core_text;
	ShaderPullFunc init_func, frame_func, use_func;
	std::map<const char*, ShaderOption*> options;
};


class Shader
{
public:
	GLuint get_id() {return id;}

	void init(ShaderProgram* program) {init_func(program);}
	void frame(ShaderProgram* program) {frame_func(program);}
	void use(ShaderProgram* program) {use_func(program);}

	ShaderCore* get_core() {return core;}
	const std::set<const char*> get_options() {return options;}

private:
	Shader(ShaderCore* core, const std::set<const char*> options);

	ShaderCore* core;
	const std::set<const char*> options;

	GLuint id;
	ShaderPullFunc init_func, frame_func, use_func;

public:
	static Shader* get(ShaderCore* core, const std::set<const char*> options);

private:
	static std::vector<Shader*> all_shaders;		//This should be a map.
};


//S3 shaders:
extern ShaderCore *vert, *geom_points, *geom_triangles, *frag_points, *frag;
//Screenspace shaders:
extern ShaderCore *vert_screenspace;

void init_shaders();



class ShaderProgram
{
public:
	GLuint get_id() {return id;}
	void use()
	{
		glUseProgram(id);
		vertex->use(this);
		if(geometry)
			geometry->use(this);
		fragment->use(this);
	}
	void init()
	{
		vertex->init(this);
		if(geometry)
			geometry->init(this);
		fragment->init(this);
	}
	void frame()
	{
		vertex->frame(this);
		if(geometry)
			geometry->frame(this);
		fragment->frame(this);
	}

	void set_matrix(const char* name, const Mat4& mat);
	void set_matrices(const char* name, const Mat4* mats, int count);
	void set_vector(const char* name, const Vec4& v);
	void set_vector(const char* name, const Vec3& v);
	void set_float(const char* name, float f);
	void set_int(const char* name, int i);
	void set_texture(const char* name, int tex_unit, GLuint texture, GLenum target = GL_TEXTURE_2D);
	void set_lut(const char* name, int tex_unit, LookupTable* lut);

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
	static std::vector<ShaderProgram*> all_shader_programs;		//This should be a map.
};
