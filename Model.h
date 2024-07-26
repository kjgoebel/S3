#pragma once

#include "Vector.h"
#include "GL/glew.h"
#include "Shaders.h"
#include "S3.h"
#include <stdio.h>


typedef std::function <void()> DrawFunc;


class Model
{
public:
	Model(int num_verts, const Vec4* verts = NULL, const Vec4* vert_colors = NULL);		//for GL_POINTS
	Model(int prim, int num_verts, int verts_per_prim, int num_prims, const Vec4* verts = NULL, const unsigned int* ixes = NULL, const Vec4* vert_colors = NULL);

	~Model();

	void dump() const;
	
	//generate_*() must be called before draw() or make_draw_func().
	//Passing scale <= 0 to generate_vertex_colors will allocate vertex_colors but not initialize it.
	void generate_vertex_colors(double scale = -1);
	void generate_primitive_colors(double scale);

	void draw(Mat4& xform, Vec4& base_color);

	DrawFunc make_draw_func(int count, Mat4* xforms, Vec4 base_color, bool use_instancing = false);
	DrawFunc make_draw_func(int count, Mat4* xforms, Vec4* base_colors, bool use_instancing = false);

	static Model* read_model_file(const char* filename, double scale);
	static Model* make_torus(int longitudinal_segments, int transverse_segments, double hole_ratio, bool use_quad_strips = true);
	static Model* make_torus_arc(int longitudinal_segments, int transverse_segments, double length, double hole_ratio, bool use_quad_strips = true);
	static Vec4* s3ify(int count, double scale, Vec3* vertices);		//Project a list of R3 vertices onto S3.

private:
	int primitive;						//GL_POINTS, GL_TRIANGLES, etc.
	int vertices_per_primitive;
	int num_vertices, num_primitives;

	Vec4* vertices;
	Vec4* vertex_colors;				//If this is NULL, the model will render with base color only.
	GLuint* elements;					//If this is NULL, use glDrawArrays() instead of glDrawElements().

	GLuint vertex_buffer, vertex_color_buffer, element_buffer;
	ShaderProgram *raw_program, *instanced_xform_program, *instanced_xform_and_color_program;		//I don't like having this many programs....
	
	GLuint raw_vertex_array;

	GLuint make_vertex_array();			//Creates a VAO and binds vertex, vertex color and element buffer objects to it as appropriate.
	void prepare_to_render();			//Creates vertex, vertex color and element buffer objects as appropriate, creates 
										//shader programs, and creates raw_vertex_array with make_vertex_array().

	void bind_xform_array(GLuint vertex_array, int count, Mat4* xforms);		//Creates a vertex buffer for the given xforms and binds it the given VAO.
	
	void draw_raw();
	void draw_instanced(int count);
};
