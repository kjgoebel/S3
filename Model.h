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

	DrawFunc make_draw_func(int count, Mat4* xforms, Vec4 base_color);
	DrawFunc make_draw_func(int count, Mat4* xforms, Vec4* base_colors);

	static Model* read_model_file(const char* filename, double scale);
	static Model* make_torus(int longitudinal_segments, int transverse_segments, double hole_ratio, bool use_quad_strips = true);
	static Model* make_torus_arc(int longitudinal_segments, int transverse_segments, double length, double hole_ratio, bool use_quad_strips = true);

private:
	int primitive;						//GL_POINTS, GL_TRIANGLES, etc.
	int vertices_per_primitive;
	int num_vertices, num_primitives;

	GLuint vertex_buffer, vertex_color_buffer, element_buffer;

	Vec4* vertices;
	Vec4* vertex_colors;				//If this is NULL, the model will render with base color only.
	GLuint* elements;					//If this is NULL, use glDrawArrays() instead of glDrawElements().

	void prepare_to_render();

	ShaderProgram *raw_program, *instanced_xform_program, *instanced_xform_and_color_program;		//I don't like having this many programs....
	GLuint raw_vertex_array;
};
