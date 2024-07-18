#pragma once

#include "Vector.h"
#include "GL/glew.h"
#include "Shaders.h"
#include "S3.h"
#include <stdio.h>


struct Model
{
	int primitive;						//GL_POINTS, GL_TRIANGLES, etc.
	int vertices_per_primitive;			//We can handle e.g. GL_QUAD_STRIP as long as each strip has the same number of vertices.
	int num_vertices, num_primitives;

	Vec4 *vertices;
	Vec4 *vertex_colors;

	unsigned int *indices;

private:
	bool ready_to_render;
	GLuint shader_program, vertex_array;

public:
	//We don't need the primitives structures for GL_POINTS:
	Model(int num_verts, const Vec4* verts = NULL, const Vec4* vert_colors = NULL);

	Model(int prim, int num_verts, int verts_per_prim, int num_prims, const Vec4* verts = NULL, const unsigned int* ixes = NULL, const Vec4* vert_colors = NULL);

	~Model();

	void draw(Mat4 xform, Vec4 baseColor);

	void dump() const;

	//Passing scale <= 0 to generate_vertex_colors will allocate vertex_colors but not initialize it.
	void generate_vertex_colors(double scale = -1);
	void generate_primitive_colors(double scale);

private:
	void prepare_to_render();

public:
	static Model* read_model_file(const char* filename, double scale);
	static Model* make_torus(int longitudinal_segments, int transverse_segments, double hole_ratio, bool use_quad_strips = true);
};
