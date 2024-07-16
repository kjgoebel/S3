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

	unsigned int *primitives;

private:
	bool ready_to_render;
	GLuint shader_program, vertex_array;

public:
	//verts_per_prim and num_prims default to zero, because primitives for GL_POINTS are trivial and don't need to be stored.
	Model(int prim, int num_verts, Vec4* verts = NULL, int verts_per_prim = 0, int num_prims = 0, int* prims = NULL, Vec4* vert_colors = NULL)
	{
		int i;
		primitive = prim;
		vertices_per_primitive = verts_per_prim;
		num_vertices = num_verts;
		num_primitives = num_prims;
		vertices = new Vec4[num_verts];
		if(verts)
			for(i = 0; i < num_verts; i++)
				vertices[i] = verts[i];
		if(verts_per_prim)
		{
			primitives = new unsigned int[num_prims * verts_per_prim];
			if(prims)
				for(i = 0; i < num_prims * verts_per_prim; i++)
					primitives[i] = prims[i];
		}
		if(vert_colors)
		{
			vertex_colors = new Vec4[num_verts];
			for(i = 0; i < num_verts; i++)
				vertex_colors[i] = vert_colors[i];
		}

		ready_to_render = false;
	}
	~Model()
	{
		delete vertices;
		if(primitives)
			delete primitives;
		if(vertex_colors)
			delete vertex_colors;
	}

	void draw(Mat4 xform, Vec4 baseColor)
	{
		if(!ready_to_render)
			prepare_to_render();

		glUseProgram(shader_program);
		glProgramUniform4f(shader_program, glGetUniformLocation(shader_program, "baseColor"), baseColor.x, baseColor.y, baseColor.z, baseColor.w);
		glProgramUniform1f(shader_program, glGetUniformLocation(shader_program, "aspectRatio"), aspect_ratio);
		glProgramUniform1f(shader_program, glGetUniformLocation(shader_program, "fogScale"), fog_scale);
		set_uniform_matrix(shader_program, "modelViewXForm", cam_mat * xform);
		set_uniform_matrix(shader_program, "projXForm", proj_mat);		//Should find a way to avoid pushing this to the GPU every frame.

		glBindVertexArray(vertex_array);

		if(primitive == GL_POINTS)
			glDrawArrays(GL_POINTS, 0, num_vertices);
		else
			for(int i = 0; i < num_primitives; i++)
				glDrawElements(primitive, vertices_per_primitive, GL_UNSIGNED_INT, (void*)(i * vertices_per_primitive * sizeof(int)));
	}

private:
	void prepare_to_render()
	{
		glGenVertexArrays(1, &vertex_array);
		glBindVertexArray(vertex_array);

		GLuint vertex_buffer, primitive_buffer;

		glGenBuffers(1, &vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_array);

		glBufferData(GL_ARRAY_BUFFER, 4 * num_vertices * sizeof(double), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_DOUBLE, GL_FALSE, 4 * sizeof(double), (void*)0);
		glEnableVertexAttribArray(0);

		switch(primitive)
		{
			case GL_POINTS:
				shader_program = make_new_program(vert, geom_points, frag_simple);
				break;
			case GL_TRIANGLES:
			case GL_QUAD_STRIP:
				shader_program = make_new_program(vert, geom_triangles, frag_simple);
				break;
			default:
				printf("Unknown primitive type %d\n", primitive);
		}

		if(primitive != GL_POINTS)		//For points we just use the vertex array directly.
		{
			glGenBuffers(1, &primitive_buffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, primitive_buffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_primitives * vertices_per_primitive * sizeof(int), primitives, GL_STATIC_DRAW);
		}

		//This is the beginning of what vertex color version is gonna look like:
		/*if(vertex_colors)
		{
			double *temp = new double[8 * num_vertices];

			for(int i = 0; i < num_vertices; i++)
				for(int j = 0; j < 4; j++)
				{
					temp[8*i + j] = vertices[i].components[j];
					temp[8*i + 4 + j] = vertex_colors[i].components[j];
				}

			glBufferData(GL_ARRAY_BUFFER, 8 * num_vertices * sizeof(double), temp, GL_STATIC_DRAW);

			delete temp;
		}*/

		ready_to_render = true;
	}
};


extern Model* geodesic_model;

void init_models();
