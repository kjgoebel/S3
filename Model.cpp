#include "Model.h"

#include <fcntl.h>
#include <io.h>
#include <stdint.h>

#include "Utils.h"


Model::Model(int num_verts, const Vec4* verts, const Vec4* vert_colors)
{
	int i;
	primitive = GL_POINTS;
	num_vertices = num_verts;
	//vertices_per_primitive = num_primitives = 0;

	//This is to make the draw code simpler.
	num_primitives = 1;
	vertices_per_primitive = num_verts;

	indices = NULL;
	vertices = new Vec4[num_verts];
	if(verts)
		for(i = 0; i < num_verts; i++)
			vertices[i] = verts[i];
	if(vert_colors)
	{
		vertex_colors = new Vec4[num_verts];
		for(i = 0; i < num_verts; i++)
			vertex_colors[i] = vert_colors[i];
	}
	else
		vertex_colors = NULL;

	ready_to_render = false;
}

Model::Model(int prim, int num_verts, int verts_per_prim, int num_prims, const Vec4* verts, const unsigned int* ixes, const Vec4* vert_colors)
{
	int i;
	primitive = prim;
	num_vertices = num_verts;
	vertices_per_primitive = verts_per_prim;
	num_primitives = num_prims;
	vertices = new Vec4[num_verts];
	if(verts)
		for(i = 0; i < num_verts; i++)
			vertices[i] = verts[i];
	if(verts_per_prim)
	{
		indices = new unsigned int[num_prims * verts_per_prim];
		if(ixes)
			for(i = 0; i < num_prims * verts_per_prim; i++)
				indices[i] = ixes[i];
	}
	if(vert_colors)
	{
		vertex_colors = new Vec4[num_verts];
		for(i = 0; i < num_verts; i++)
			vertex_colors[i] = vert_colors[i];
	}
	else
		vertex_colors = NULL;

	ready_to_render = false;
}

Model::~Model()
{
	delete vertices;
	if(indices)
		delete indices;
	if(vertex_colors)
		delete vertex_colors;
}

void Model::prepare_to_render()
{
	glGenVertexArrays(1, &vertex_array);
	glBindVertexArray(vertex_array);

	GLuint vertex_buffer;

	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

	glBufferData(GL_ARRAY_BUFFER, num_vertices * sizeof(Vec4), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_DOUBLE, GL_FALSE, sizeof(Vec4), (void*)0);
	glEnableVertexAttribArray(0);

	if(vertex_colors)
	{
		GLuint color_buffer;

		glGenBuffers(1, &color_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, color_buffer);

		glBufferData(GL_ARRAY_BUFFER, num_vertices * sizeof(Vec4), vertex_colors, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 4, GL_DOUBLE, GL_FALSE, sizeof(Vec4), (void*)0);
		glEnableVertexAttribArray(1);
	}

	if(indices)		//For points and models with one vertex per vertex-on-a-primitive, we just use the vertex array directly.
	{
		GLuint element_buffer;
		glGenBuffers(1, &element_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_primitives * vertices_per_primitive * sizeof(int), indices, GL_STATIC_DRAW);
	}

	GLuint geom_shader, frag_shader = vertex_colors ? frag_vcolor : frag_simple;
	switch(primitive)
	{
		case GL_POINTS:
			geom_shader = geom_points;
			break;
		default:
			geom_shader = geom_triangles;
			break;
	}

	shader_program = make_new_program(vert, geom_shader, frag_shader);

	ready_to_render = true;
}

void Model::draw(Mat4 xform, Vec4 baseColor)
{
	if(!ready_to_render)
		prepare_to_render();

	glUseProgram(shader_program);
	glProgramUniform4f(shader_program, glGetUniformLocation(shader_program, "baseColor"), baseColor.x, baseColor.y, baseColor.z, baseColor.w);
	glProgramUniform1f(shader_program, glGetUniformLocation(shader_program, "aspectRatio"), aspect_ratio);
	glProgramUniform1f(shader_program, glGetUniformLocation(shader_program, "fogScale"), fog_scale);
	set_uniform_matrix(shader_program, "modelViewXForm", ~cam_mat * xform);		//That should be the inverse of cam_mat, but it _should_ always be SO(4), so the inverse _should_ always be the transpose....
	set_uniform_matrix(shader_program, "projXForm", proj_mat);		//Should find a way to avoid pushing this to the GPU every frame.

	glBindVertexArray(vertex_array);

	if(indices)
		switch(primitive)
		{
			case GL_LINES:
			case GL_TRIANGLES:
			case GL_QUADS:
				glDrawElements(primitive, vertices_per_primitive * num_primitives, GL_UNSIGNED_INT, (void*)0);
				break;
			default:
				for(int i = 0; i < num_primitives; i++)
					glDrawElements(primitive, vertices_per_primitive, GL_UNSIGNED_INT, (void*)(i * vertices_per_primitive * sizeof(int)));
				break;
		}
	else
		for(int i = 0; i < num_primitives; i++)
			glDrawArrays(primitive, i * vertices_per_primitive, vertices_per_primitive);
}


void Model::dump() const
{
	int i;
	printf("%d %d %d %d\n", primitive, num_vertices, vertices_per_primitive, num_primitives);
	for(i = 0; i < num_vertices; i++)
		printf("%d: %f %f %f %f\n", i, vertices[i].x, vertices[i].y, vertices[i].z, vertices[i].w);
	if(indices)
		for(i = 0; i < num_primitives; i++)
		{
			for(int j = 0; j < vertices_per_primitive; j++)
				printf("%d ", indices[i * vertices_per_primitive + j]);
			printf("\n");
		}
	else
		printf("no indices");
}


Model* Model::read_model_file(const char* filename, double scale)
{
	int i;
	uint32_t dummy[3];
	uint32_t num_verts, num_edges, num_triangles;
	Vec3 temp;

	int fin = _open(filename, O_RDONLY | O_BINARY);
	
	_read(fin, &num_verts, 4);
	_read(fin, &num_edges, 4);
	_read(fin, &num_triangles, 4);

	printf("%d, %d, %d\n", num_verts, num_edges, num_triangles);

	Vec4* verts = new Vec4[num_verts];
	unsigned int* ixes = new unsigned int[3 * num_triangles];

	for(i = 0; i < num_verts; i++)
	{
		_read(fin, &temp.components, sizeof(temp.components));
		verts[i] = Vec4(
			temp.x * scale,
			temp.y * scale,
			temp.z * scale,
			1
		).normalize();
		printf("%d (%f %f %f) -> (%f %f %f %f)\n", i,
			temp.x, temp.y, temp.z,
			verts[i].x, verts[i].y, verts[i].z, verts[i].w
		);
	}

	for(i = 0; i < num_edges; i++)
		_read(fin, dummy, 8);

	for(i = 0; i < num_triangles; i++)
	{
		_read(fin, &ixes[i * 3], 12);
		_read(fin, dummy, 12);
		printf("%d (%d %d %d)\n", i,
			ixes[3 * i], ixes[3 * i + 1], ixes[3 * i + 2]
		);
	}

	_close(fin);

	Model* ret = new Model(GL_TRIANGLES, num_verts, 3, num_triangles, verts, ixes);
	delete verts;
	delete ixes;
	return ret;
}


void Model::generate_vertex_colors(double scale)
{
	if(vertex_colors)
		error("Model already has vertex colors.");
	vertex_colors = new Vec4[num_vertices];
	if(scale > 0)
		for(int i = 0; i < num_vertices; i++)
			vertex_colors[i] = Vec4(scale * frand(), scale * frand(), scale * frand(), 0);
}

void Model::generate_primitive_colors(double scale)
{
	if(vertex_colors)
		error("Model already has vertex colors.");

	Vec4* old_verts = vertices;
	num_vertices = num_primitives * vertices_per_primitive;
	vertices = new Vec4[num_vertices];

	unsigned int* old_prims = indices;
	indices = NULL;

	vertex_colors = new Vec4[num_vertices];

	for(int i = 0; i < num_primitives; i++)
	{
		Vec4 temp(scale * frand(), scale * frand(), scale * frand(), 0);
		for(int j = 0; j < vertices_per_primitive; j++)
		{
			int ix = vertices_per_primitive * i + j;
			vertices[ix] = old_verts[old_prims[ix]];
			vertex_colors[ix] = temp;
		}
	}

	delete old_verts;
	delete old_prims;
}


Vec4* make_torus_verts(int long_segments, int trans_segments, double hole_ratio)
{
	double normalization_factor = 1.0 / sqrt(1.0 + hole_ratio * hole_ratio);

	Vec4* ret = new Vec4[long_segments * trans_segments];
	for(int i = 0; i < long_segments; i++)
	{
		double theta = (double)i * TAU / long_segments;
		for(int j = 0; j < trans_segments; j++)
		{
			double phi = (double)j * TAU / trans_segments;

			ret[i * trans_segments + j] = normalization_factor * Vec4(
				hole_ratio * cos(phi),
				hole_ratio * sin(phi),
				cos(theta),
				sin(theta)
			);
		}
	}

	return ret;
}

unsigned int* make_torus_quad_strip_indices(int long_segments, int trans_segments)
{
	int verts_per_strip = 2 * (trans_segments + 1);
	unsigned int* ret = new unsigned int[long_segments * verts_per_strip];

	/*
		Note: I was going to try making the strips run longitudinally, so that 
		it'd be more efficient (assuming more longitudinal segments than 
		transverse), but I found that putting rings on the geodesics made 
		them look better and made it easier to see what they were doing, 
		so transverse strips it is.
	*/

	for(int i = 0; i < long_segments; i++)
	{
		for(int j = 0; j < trans_segments; j++)
		{
			ret[i * verts_per_strip + 2 * j] = i * trans_segments + j;
			ret[i * verts_per_strip + 2 * j + 1] = ((i + 1) % long_segments) * trans_segments + j;
		}

		ret[i * verts_per_strip + 2 * trans_segments] = i * trans_segments;
		ret[i * verts_per_strip + 2 * trans_segments + 1] = ((i + 1) % long_segments) * trans_segments;
	}

	return ret;
}

unsigned int* make_torus_quad_indices(int long_segments, int trans_segments)
{
	unsigned int* ret = new unsigned int[4 * long_segments * trans_segments];

	for(int i = 0; i < long_segments; i++)
		for(int j = 0; j < trans_segments; j++)
		{
			ret[4 * (i * trans_segments + j)] = i * trans_segments + j;
			ret[4 * (i * trans_segments + j) + 1] = ((i + 1) % long_segments) * trans_segments + j;
			ret[4 * (i * trans_segments + j) + 2] = ((i + 1) % long_segments) * trans_segments + ((j + 1) % trans_segments);
			ret[4 * (i * trans_segments + j) + 3] = i * trans_segments + ((j + 1) % trans_segments);
		}

	return ret;
}


Model* Model::make_torus(int longitudinal_segments, int transverse_segments, double hole_ratio, bool use_quad_strips)
{
	if(use_quad_strips)
		return new Model(
			GL_QUAD_STRIP,
			longitudinal_segments * transverse_segments,
			2 * (transverse_segments + 1),
			longitudinal_segments,
			make_torus_verts(longitudinal_segments, transverse_segments, hole_ratio),
			make_torus_quad_strip_indices(longitudinal_segments, transverse_segments)
		);
	else
		return new Model(
			GL_QUADS,
			longitudinal_segments * transverse_segments,
			4,
			longitudinal_segments * transverse_segments,
			make_torus_verts(longitudinal_segments, transverse_segments, hole_ratio),
			make_torus_quad_indices(longitudinal_segments, transverse_segments)
		);
}



