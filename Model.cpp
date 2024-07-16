#include "Model.h"


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

	GLuint geom_shader, frag_shader = frag_simple;		//Fragment shader will vary in the future.
	switch(primitive)
	{
		case GL_POINTS:
			geom_shader = geom_points;
			break;
		case GL_TRIANGLES:
		case GL_QUAD_STRIP:
			geom_shader = geom_triangles;
			break;
		default:
			printf("Unknown primitive type %d\n", primitive);
			exit(-1);
			break;
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

	if(primitive == GL_POINTS)
		glDrawArrays(GL_POINTS, 0, num_vertices);
	else
		for(int i = 0; i < num_primitives; i++)
			glDrawElements(primitive, vertices_per_primitive, GL_UNSIGNED_INT, (void*)(i * vertices_per_primitive * sizeof(int)));
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
				sin(theta),
				cos(theta)
			);
			
			printf("%d: %d\n", i * verts_per_strip + 2 * j, i * GEODESIC_TRANSVERSE_SEGMENTS + j);
			printf("%d: %d\n", i * verts_per_strip + 2 * j + 1, ((i + 1) % GEODESIC_LONGITUDINAL_SEGMENTS) * GEODESIC_TRANSVERSE_SEGMENTS + j);
			geodesic_model->primitives[i * verts_per_strip + 2 * j] = i * GEODESIC_TRANSVERSE_SEGMENTS + j;
			geodesic_model->primitives[i * verts_per_strip + 2 * j + 1] = ((i + 1) % GEODESIC_LONGITUDINAL_SEGMENTS) * GEODESIC_TRANSVERSE_SEGMENTS + j;
		}
		
		printf("%d: %d\n", i * verts_per_strip + 2 * GEODESIC_TRANSVERSE_SEGMENTS, i * GEODESIC_TRANSVERSE_SEGMENTS);
		printf("%d: %d\n", i * verts_per_strip + 2 * GEODESIC_TRANSVERSE_SEGMENTS + 1, ((i + 1) % GEODESIC_LONGITUDINAL_SEGMENTS) * GEODESIC_TRANSVERSE_SEGMENTS);
		geodesic_model->primitives[i * verts_per_strip + 2 * GEODESIC_TRANSVERSE_SEGMENTS] = i * GEODESIC_TRANSVERSE_SEGMENTS;
		geodesic_model->primitives[i * verts_per_strip + 2 * GEODESIC_TRANSVERSE_SEGMENTS + 1] = ((i + 1) % GEODESIC_LONGITUDINAL_SEGMENTS) * GEODESIC_TRANSVERSE_SEGMENTS;
	}

	printf("%d %d\n", geodesic_model->num_vertices, geodesic_model->num_primitives * geodesic_model->vertices_per_primitive);

	for(int i = 0; i < geodesic_model->num_vertices; i++)
		print_vector(geodesic_model->vertices[i]);
	for(int i = 0; i < geodesic_model->num_primitives; i++)
	{
		for(int j = 0; j < geodesic_model->vertices_per_primitive; j++)
			printf("%d  ", geodesic_model->primitives[i * geodesic_model->vertices_per_primitive + j]);
		printf("\n");
	}
	printf("Done.\n");
	//exit(0);
}
