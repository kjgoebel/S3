#include "Model.h"

#include <fcntl.h>
#include <io.h>
#include <stdint.h>

#include "Utils.h"


Model* geodesic_model = NULL;


//Depth is still messed up for near-antipodal elements.

Model::Model(int num_verts, const Vec4* verts, const Vec4* vert_colors)
{
	int i;
	primitive = GL_POINTS;
	num_vertices = num_verts;
	vertices_per_primitive = num_primitives = 0;
	primitives = NULL;
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

Model::Model(int prim, int num_verts, int verts_per_prim = 0, int num_prims = 0, const Vec4* verts, const int* prims, const Vec4* vert_colors)
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
	else
		vertex_colors = NULL;

	ready_to_render = false;
}

Model::~Model()
{
	delete vertices;
	if(primitives)
		delete primitives;
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

	if(primitive != GL_POINTS)		//For points we just use the vertex array directly.
	{
		GLuint element_buffer;
		glGenBuffers(1, &element_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_primitives * vertices_per_primitive * sizeof(int), primitives, GL_STATIC_DRAW);
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

void Model::draw(Mat4 xform, Vec4 baseColor)
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

	switch(primitive)
	{
		case GL_POINTS:
			glDrawArrays(GL_POINTS, 0, num_vertices);
			break;
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
}


void Model::dump() const
{
	int i;
	printf("%d %d %d %d\n", primitive, num_vertices, vertices_per_primitive, num_primitives);
	for(i = 0; i < num_vertices; i++)
		printf("%d: %f %f %f %f\n", i, vertices[i].x, vertices[i].y, vertices[i].z, vertices[i].w);
	for(i = 0; i < num_primitives; i++)
	{
		for(int j = 0; j < vertices_per_primitive; j++)
			printf("%d ", primitives[i * vertices_per_primitive + j]);
		printf("\n");
	}
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

	Model* ret = new Model(GL_TRIANGLES, num_verts, 3, num_triangles);

	for(i = 0; i < num_verts; i++)
	{
		_read(fin, &temp.components, sizeof(temp.components));
		ret->vertices[i] = Vec4(
			temp.x * scale,
			temp.y * scale,
			temp.z * scale,
			1
		).normalize();
		printf("%d (%f %f %f) -> (%f %f %f %f)\n", i,
			temp.x, temp.y, temp.z,
			ret->vertices[i].x, ret->vertices[i].y, ret->vertices[i].z, ret->vertices[i].w
		);
	}

	for(i = 0; i < num_edges; i++)
		_read(fin, dummy, 8);

	for(i = 0; i < num_triangles; i++)
	{
		_read(fin, &ret->primitives[i * 3], 12);
		_read(fin, dummy, 12);
		printf("%d (%d %d %d)\n", i,
			ret->primitives[3 * i], ret->primitives[3 * i + 1], ret->primitives[3 * i + 2]
		);
	}

	_close(fin);

	ret->vertex_colors = new Vec4[ret->num_vertices];
	for(i = 0; i < ret->num_vertices; i++)
		ret->vertex_colors[i] = Vec4(0.3 * frand(), 0.3 * frand(), 0.3 * frand(), 0);

	return ret;
}


#define GEODESIC_TUBE_RADIUS			(0.005)
#define GEODESIC_LONGITUDINAL_SEGMENTS	(128)
#define GEODESIC_TRANSVERSE_SEGMENTS	(8)

void init_models()
{
	int verts_per_strip = 2 * (GEODESIC_TRANSVERSE_SEGMENTS + 1);

	geodesic_model = new Model(
		GL_QUAD_STRIP,
		GEODESIC_LONGITUDINAL_SEGMENTS * GEODESIC_TRANSVERSE_SEGMENTS,
		verts_per_strip,
		GEODESIC_LONGITUDINAL_SEGMENTS
	);

	double normalization_factor = 1.0 / sqrt(1.0 + GEODESIC_TUBE_RADIUS * GEODESIC_TUBE_RADIUS);
	for(int i = 0; i < GEODESIC_LONGITUDINAL_SEGMENTS; i++)
	{
		double theta = (double)i * TAU / GEODESIC_LONGITUDINAL_SEGMENTS;
		for(int j = 0; j < GEODESIC_TRANSVERSE_SEGMENTS; j++)
		{
			double phi = (double)j * TAU / GEODESIC_TRANSVERSE_SEGMENTS;
			geodesic_model->vertices[i * GEODESIC_TRANSVERSE_SEGMENTS + j] = normalization_factor * Vec4(
				GEODESIC_TUBE_RADIUS * cos(phi),
				GEODESIC_TUBE_RADIUS * sin(phi),
				cos(theta),
				sin(theta)
			);
			
			geodesic_model->primitives[i * verts_per_strip + 2 * j] = i * GEODESIC_TRANSVERSE_SEGMENTS + j;
			geodesic_model->primitives[i * verts_per_strip + 2 * j + 1] = ((i + 1) % GEODESIC_LONGITUDINAL_SEGMENTS) * GEODESIC_TRANSVERSE_SEGMENTS + j;
		}
		
		geodesic_model->primitives[i * verts_per_strip + 2 * GEODESIC_TRANSVERSE_SEGMENTS] = i * GEODESIC_TRANSVERSE_SEGMENTS;
		geodesic_model->primitives[i * verts_per_strip + 2 * GEODESIC_TRANSVERSE_SEGMENTS + 1] = ((i + 1) % GEODESIC_LONGITUDINAL_SEGMENTS) * GEODESIC_TRANSVERSE_SEGMENTS;
	}
}
