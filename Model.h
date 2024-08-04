#pragma once

#include "Vector.h"
#include "GL/glew.h"
#include "Shaders.h"
#include "Camera.h"
#include <stdio.h>
#include <memory>


typedef std::function <void()> DrawFunc;


class Model
{
public:
	Model(int num_verts, const Vec4* verts = NULL, const Vec4* vert_colors = NULL);		//for GL_POINTS
	Model(
		int prim,
		int num_verts,
		int verts_per_prim,
		int num_prims,
		const Vec4* verts = NULL,
		const unsigned int* ixes = NULL,
		const Vec4* vert_colors = NULL,
		const Vec4* norms = NULL
	);

	~Model();

	void dump() const;
	
	//generate_*() must be called before draw(), make_draw_func() or prepare_to_render().
	//Passing scale <= 0 to generate_vertex_colors will allocate vertex_colors but not initialize it.
	void generate_vertex_colors(double scale = -1);
	/*
		Generate vertex colors such that each primitive is a solid color. This necessitates splitting 
		each vertex into as many versions of itself as there are primitives that include it. Note that 
		if normals are present, they are preserved for each vertex as they were before the splitting 
		happened. So, if you want smooth shading with colored primitives, give the model normals 
		before calling generate_primitive_colors(), and if you want flat shading with colored 
		primitives, call generate_normals() after calling generate_primitive_colors().
	*/
	void generate_primitive_colors(double scale);

	void generate_normals();

	void draw(const Mat4& xform, const Vec4& base_color);

	//Note: Instancing is broken. Dunno why, but passing use_instancing = true makes rendering much slower.
	DrawFunc make_draw_func(int count, const Mat4* xforms, Vec4 base_color, bool use_instancing = false);
	DrawFunc make_draw_func(int count, const Mat4* xforms, const Vec4* base_colors, bool use_instancing = false);
	
	static Model* make_icosahedron(double scale, int subdivisions = 0, bool normalize = false);
	static Model* make_torus(int longitudinal_segments, int transverse_segments, double hole_ratio, bool use_quad_strips = true, bool make_normals = false);
	static Model* make_torus_arc(int longitudinal_segments, int transverse_segments, double length, double hole_ratio, bool use_quad_strips = true, bool make_normals = false);
	static Model* make_bumpy_torus(int longitudinal_segments, int transverse_segments, double bump_height, bool use_quad_strips = false);
	static std::shared_ptr<Vec4[]> s3ify(int count, double scale, const Vec3* vertices);		//Project a list of R3 vertices onto S3.
	
	/*
		Creates buffers and raw_vertex_array. Called automatically as needed by 
		draw and make_draw_func(). Exposed as public because for some #$&(@ 
		reason it sometimes causes segfaults unless it's manually called 
		earlier.
	*/
	void prepare_to_render();

private:
	int primitive;						//GL_POINTS, GL_TRIANGLES, etc.
	int vertices_per_primitive;
	int num_vertices, num_primitives;

	std::unique_ptr<Vec4[]> vertices;
	std::unique_ptr<Vec4[]> vertex_colors;				//If this is NULL, the model will render with base color only.
	std::unique_ptr<GLuint[]> elements;					//If this is NULL, use glDrawArrays() instead of glDrawElements().
	std::unique_ptr<Vec4[]> normals;					//If this is NULL, lighting calculation will ignore effects of surface normal (only distance and shadow will be used).

	GLuint vertex_buffer, vertex_color_buffer, element_buffer, normal_buffer;
	
	GLuint raw_vertex_array;

	GLuint make_vertex_array();			//Creates a VAO and binds vertex, vertex color and element buffer objects to it as appropriate.

	ShaderProgram* get_shader_program(bool shadow, bool instanced_xforms, bool instanced_base_colors);

	void bind_xform_array(GLuint vertex_array, int count, const Mat4* xforms);		//Creates a vertex buffer for the given xforms and binds it the given VAO.
	void bind_color_array(GLuint vertex_array, int count, const Vec4* base_colors);
	
	void draw_raw();
	void draw_instanced(int count);
};
