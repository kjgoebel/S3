#include "Model.h"

#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <memory>

#include "Utils.h"

#pragma warning(disable : 4244)		//conversion from double to float


Model::Model(int num_verts, const Vec4* verts, const Vec4* vert_colors)
{
	int i;
	primitive = GL_POINTS;
	num_vertices = num_verts;

	//This is to make the draw code simpler.
	num_primitives = 1;
	vertices_per_primitive = num_verts;

	elements = NULL;
	vertices = std::unique_ptr<Vec4[]>(new Vec4[num_verts]);
	if(verts)
		for(i = 0; i < num_verts; i++)
			vertices[i] = verts[i];
	if(vert_colors)
	{
		vertex_colors = std::unique_ptr<Vec4[]>(new Vec4[num_verts]);
		for(i = 0; i < num_verts; i++)
			vertex_colors[i] = vert_colors[i];
	}
	else
		vertex_colors = NULL;

	vertex_buffer = vertex_color_buffer = element_buffer = 0;
	raw_program = instanced_xform_program = instanced_xform_and_color_program = NULL;
	raw_vertex_array = 0;
}

Model::Model(int prim, int num_verts, int verts_per_prim, int num_prims, const Vec4* verts, const unsigned int* ixes, const Vec4* vert_colors)
{
	int i;
	primitive = prim;
	num_vertices = num_verts;
	vertices_per_primitive = verts_per_prim;
	num_primitives = num_prims;
	vertices = std::unique_ptr<Vec4[]>(new Vec4[num_verts]);
	if(verts)
		for(i = 0; i < num_verts; i++)
			vertices[i] = verts[i];
	if(verts_per_prim)
	{
		elements = std::unique_ptr<GLuint[]>(new GLuint[num_prims * verts_per_prim]);
		if(ixes)
			for(i = 0; i < num_prims * verts_per_prim; i++)
				elements[i] = ixes[i];
	}
	else
		elements = NULL;
	if(vert_colors)
	{
		vertex_colors = std::unique_ptr<Vec4[]>(new Vec4[num_verts]);
		for(i = 0; i < num_verts; i++)
			vertex_colors[i] = vert_colors[i];
	}
	else
		vertex_colors = NULL;

	vertex_buffer = vertex_color_buffer = element_buffer = 0;
	raw_program = instanced_xform_program = instanced_xform_and_color_program = NULL;
	raw_vertex_array = 0;
}

Model::~Model()
{
	if(vertex_buffer)
		glDeleteBuffers(1, &vertex_buffer);
	if(vertex_color_buffer)
		glDeleteBuffers(1, &vertex_color_buffer);
	if(element_buffer)
		glDeleteBuffers(1, &element_buffer);
	if(raw_vertex_array)
		glDeleteVertexArrays(1, &raw_vertex_array);
}


void Model::prepare_to_render()
{
	if(vertex_buffer)
		error("Model was already prepared for rendering.\n");

	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, num_vertices * sizeof(Vec4), vertices.get(), GL_STATIC_DRAW);

	if(vertex_colors)
	{
		glGenBuffers(1, &vertex_color_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_color_buffer);
		glBufferData(GL_ARRAY_BUFFER, num_vertices * sizeof(Vec4), vertex_colors.get(), GL_STATIC_DRAW);
	}

	if(elements)
	{
		glGenBuffers(1, &element_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_primitives * vertices_per_primitive * sizeof(int), elements.get(), GL_STATIC_DRAW);
	}

	auto options = vertex_colors ? std::set<const char*>{DEFINE_VERTEX_COLOR} : std::set<const char*>{};
	raw_program = ShaderProgram::get(
		Shader::get(vert, options),
		Shader::get(primitive == GL_POINTS ? geom_points : geom_triangles, options),
		Shader::get(frag, options)
	);

	auto vert_options = options;
	vert_options.insert(DEFINE_INSTANCED_XFORM);
	instanced_xform_program = ShaderProgram::get(
		Shader::get(vert, vert_options),
		Shader::get(primitive == GL_POINTS ? geom_points : geom_triangles, options),
		Shader::get(frag, options)
	);

	vert_options.insert(DEFINE_INSTANCED_BASE_COLOR);
	options.insert(DEFINE_INSTANCED_BASE_COLOR);
	instanced_xform_and_color_program = ShaderProgram::get(
		Shader::get(vert, vert_options),
		Shader::get(primitive == GL_POINTS ? geom_points : geom_triangles, options),
		Shader::get(frag, options)
	);

	raw_vertex_array = make_vertex_array();
}

GLuint Model::make_vertex_array()
{
	GLuint vertex_array;
	glGenVertexArrays(1, &vertex_array);
	glBindVertexArray(vertex_array);

	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 4, GL_DOUBLE, GL_FALSE, sizeof(Vec4), (void*)0);
	glEnableVertexAttribArray(0);

	if(vertex_color_buffer)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vertex_color_buffer);
		glVertexAttribPointer(1, 4, GL_DOUBLE, GL_FALSE, sizeof(Vec4), (void*)0);
		glEnableVertexAttribArray(1);
	}

	if(elements)
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);

	glBindVertexArray(0);

	return vertex_array;
}


void Model::draw(const Mat4& xform, const Vec4& base_color)
{
	if(!vertex_buffer)
		prepare_to_render();
		
	raw_program->use();
	GLuint program_id = raw_program->get_id();
	glProgramUniform4f(program_id, glGetUniformLocation(program_id, "baseColor"), base_color.x, base_color.y, base_color.z, base_color.w);
	raw_program->set_uniform_matrix("modelViewXForm", ~cam_mat * xform);		//That should be the inverse of cam_mat, but it _should_ always be SO(4), so the inverse _should_ always be the transpose....

	glBindVertexArray(raw_vertex_array);

	draw_raw();
}


void Model::bind_xform_array(GLuint vertex_array, int count, const Mat4* xforms)
{
	glBindVertexArray(vertex_array);

	GLuint xform_buffer;
	glGenBuffers(1, &xform_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, xform_buffer);

	float* temp = new float[16 * count];
	for(int mat = 0; mat < count; mat++)
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
				temp[mat * 16 + j * 4 + i] = xforms[mat].data[i][j];
	glBufferData(GL_ARRAY_BUFFER, count * 16 * sizeof(float), temp, GL_STATIC_DRAW);
	delete[] temp;

	for(int i = 0; i < 4; i++)
	{
		glEnableVertexAttribArray(2 + i);
		glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)(4 * i * sizeof(float)));
		glVertexAttribDivisor(2 + i, 1);
	}

	glBindVertexArray(0);
}

void Model::draw_raw()
{
	if(elements)
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

void Model::draw_instanced(int count)
{
	if(elements)
		switch(primitive)
		{
			case GL_LINES:
			case GL_TRIANGLES:
			case GL_QUADS:
				glDrawElementsInstanced(primitive, vertices_per_primitive * num_primitives, GL_UNSIGNED_INT, (void*)0, count);
				break;
			default:
				for(int j = 0; j < num_primitives; j++)
					glDrawElementsInstanced(primitive, vertices_per_primitive, GL_UNSIGNED_INT, (void*)(j * vertices_per_primitive * sizeof(int)), count);
				break;
		}
	else
		for(int j = 0; j < num_primitives; j++)
			glDrawArraysInstanced(primitive, j * vertices_per_primitive, vertices_per_primitive, count);
}


DrawFunc Model::make_draw_func(int count, const Mat4* xforms, Vec4 base_color, bool use_instancing)
{
	if(!vertex_buffer)
		prepare_to_render();

	if(use_instancing)
	{
		GLuint vertex_array = make_vertex_array();
		bind_xform_array(vertex_array, count, xforms);

		glBindVertexArray(0);

		return [count, vertex_array, base_color, this]() {
			instanced_xform_program->use();
			glBindVertexArray(vertex_array);
			glProgramUniform4f(
				instanced_xform_program->get_id(),
				glGetUniformLocation(instanced_xform_program->get_id(), "baseColor"),
				base_color.x, base_color.y, base_color.z, base_color.w
			);
			draw_instanced(count);
			glBindVertexArray(0);
		};
	}
	else
	{
		std::shared_ptr<Mat4[]> temp_xforms(new Mat4[count]);
		for(int i = 0; i < count; i++)
			temp_xforms[i] = xforms[i];

		return [count, temp_xforms, base_color, this]() {
			raw_program->use();
			GLuint program_id = raw_program->get_id();
			glProgramUniform4f(program_id, glGetUniformLocation(program_id, "baseColor"), base_color.x, base_color.y, base_color.z, base_color.w);

			for(int i = 0; i < count; i++)
			{
				raw_program->set_uniform_matrix("modelViewXForm", ~cam_mat * temp_xforms[i]);
				glBindVertexArray(raw_vertex_array);
				draw_raw();
			}
		};
	}
}


DrawFunc Model::make_draw_func(int count, const Mat4* xforms, const Vec4* base_colors, bool use_instancing)
{
	if(!vertex_buffer)
		prepare_to_render();

	if(use_instancing)
	{
		GLuint vertex_array = make_vertex_array();
		bind_xform_array(vertex_array, count, xforms);

		glBindVertexArray(vertex_array);

		GLuint base_color_buffer;
		glGenBuffers(1, &base_color_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, base_color_buffer);
		glBufferData(GL_ARRAY_BUFFER, count * sizeof(Vec4), base_colors, GL_STATIC_DRAW);
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_DOUBLE, GL_FALSE, sizeof(Vec4), (void*)0);
		glVertexAttribDivisor(6, 1);

		glBindVertexArray(0);

		return [count, vertex_array, this]() {
			instanced_xform_and_color_program->use();
			glBindVertexArray(vertex_array);
			draw_instanced(count);
			glBindVertexArray(0);
		};
	}
	else
	{
		std::shared_ptr<Mat4[]> temp_xforms(new Mat4[count]);
		std::shared_ptr<Vec4[]> temp_colors(new Vec4[count]);
		for(int i = 0; i < count; i++)
		{
			temp_xforms[i] = xforms[i];
			temp_colors[i] = base_colors[i];
		}

		return [count, temp_xforms, temp_colors, this]() {
			raw_program->use();
			GLuint program_id = raw_program->get_id();
			for(int i = 0; i < count; i++)
			{
				Vec4 base_color = temp_colors[i];
				glProgramUniform4f(program_id, glGetUniformLocation(program_id, "baseColor"), base_color.x, base_color.y, base_color.z, base_color.w);
				raw_program->set_uniform_matrix("modelViewXForm", ~cam_mat * temp_xforms[i]);
				glBindVertexArray(raw_vertex_array);
				draw_raw();
			}
		};
	}
}


void Model::dump() const
{
	int i;
	printf("%d %d %d %d\n", primitive, num_vertices, vertices_per_primitive, num_primitives);
	for(i = 0; i < num_vertices; i++)
		printf("%d: %f %f %f %f\n", i, vertices[i].x, vertices[i].y, vertices[i].z, vertices[i].w);
	if(elements)
		for(i = 0; i < num_primitives; i++)
		{
			for(int j = 0; j < vertices_per_primitive; j++)
				printf("%d ", elements[i * vertices_per_primitive + j]);
			printf("\n");
		}
	else
		printf("no elements");
}


void Model::generate_vertex_colors(double scale)
{
	if(vertex_colors)
		error("Model already has vertex colors.");
	vertex_colors = std::unique_ptr<Vec4[]>(new Vec4[num_vertices]);
	if(scale > 0)
		for(int i = 0; i < num_vertices; i++)
			vertex_colors[i] = Vec4(scale * frand(), scale * frand(), scale * frand(), 0);
}

void Model::generate_primitive_colors(double scale)
{
	if(vertex_colors)
		error("Model already has vertex colors.");

	std::unique_ptr<Vec4[]> old_verts = std::move(vertices);
	num_vertices = num_primitives * vertices_per_primitive;
	vertices.reset(new Vec4[num_vertices]);

	std::unique_ptr<GLuint[]> old_elements = std::move(elements);
	elements = NULL;

	vertex_colors = std::unique_ptr<Vec4[]>(new Vec4[num_vertices]);

	for(int i = 0; i < num_primitives; i++)
	{
		Vec4 temp(scale * frand(), scale * frand(), scale * frand(), 0);
		for(int j = 0; j < vertices_per_primitive; j++)
		{
			int ix = vertices_per_primitive * i + j;
			vertices[ix] = old_verts[old_elements[ix]];
			vertex_colors[ix] = temp;
		}
	}
}


std::shared_ptr<Vec4[]> make_torus_verts(int long_segments, int trans_segments, double hole_ratio, double length = TAU, bool make_final_ring = false)
{
	double normalization_factor = 1.0 / sqrt(1.0 + hole_ratio * hole_ratio);

	std::shared_ptr<Vec4[]> ret(new Vec4[long_segments * trans_segments]);
	for(int i = 0; i < long_segments; i++)
	{
		double theta = (double)i * length / (make_final_ring ? long_segments - 1 : long_segments);
		for(int j = 0; j < trans_segments; j++)
		{
			double phi = (double)j * TAU / trans_segments;

			ret[i * trans_segments + j] = normalization_factor * Vec4(
				hole_ratio * sin(phi),
				hole_ratio * cos(phi),
				sin(theta),
				cos(theta)
			);
		}
	}

	return ret;
}

std::shared_ptr<GLuint[]> make_torus_quad_strip_indices(int long_segments, int trans_segments, bool loop_longitudinally = true)
{
	int verts_per_strip = 2 * (trans_segments + 1);
	std::shared_ptr<GLuint[]> ret(new GLuint[long_segments * verts_per_strip]);

	/*
		Note: I was going to try making the strips run longitudinally, so that 
		it'd be more efficient (assuming more longitudinal segments than 
		transverse), but I found that putting rings on the geodesics made 
		them look better and made it easier to see what they were doing, 
		so transverse strips it is.
	*/

	for(int i = 0; i < long_segments; i++)
	{
		#define NEXT_I (loop_longitudinally ? (i + 1) % long_segments : i + 1)
		for(int j = 0; j < trans_segments; j++)
		{
			ret[i * verts_per_strip + 2 * j] = i * trans_segments + j;
			ret[i * verts_per_strip + 2 * j + 1] = NEXT_I * trans_segments + j;
		}

		ret[i * verts_per_strip + 2 * trans_segments] = i * trans_segments;
		ret[i * verts_per_strip + 2 * trans_segments + 1] = NEXT_I * trans_segments;
		#undef NEXT_I
	}

	return ret;
}

std::shared_ptr<GLuint[]> make_torus_quad_indices(int long_segments, int trans_segments, bool loop_longitudinally = true)
{
	std::shared_ptr<GLuint[]> ret(new GLuint[4 * long_segments * trans_segments]);

	for(int i = 0; i < long_segments; i++)
		for(int j = 0; j < trans_segments; j++)
		{
			#define NEXT_I (loop_longitudinally ? (i + 1) % long_segments : i + 1)
			ret[4 * (i * trans_segments + j)] = i * trans_segments + j;
			ret[4 * (i * trans_segments + j) + 1] = NEXT_I * trans_segments + j;
			ret[4 * (i * trans_segments + j) + 2] = NEXT_I * trans_segments + ((j + 1) % trans_segments);
			ret[4 * (i * trans_segments + j) + 3] = i * trans_segments + ((j + 1) % trans_segments);
			#undef NEXT_I
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
			make_torus_verts(longitudinal_segments, transverse_segments, hole_ratio).get(),
			make_torus_quad_strip_indices(longitudinal_segments, transverse_segments).get()
		);
	else
		return new Model(
			GL_QUADS,
			longitudinal_segments * transverse_segments,
			4,
			longitudinal_segments * transverse_segments,
			make_torus_verts(longitudinal_segments, transverse_segments, hole_ratio).get(),
			make_torus_quad_indices(longitudinal_segments, transverse_segments).get()
		);
}


Model* Model::make_torus_arc(int longitudinal_segments, int transverse_segments, double length, double hole_ratio, bool use_quad_strips)
{
	if(use_quad_strips)
		return new Model(
			GL_QUAD_STRIP,
			longitudinal_segments * transverse_segments,
			2 * (transverse_segments + 1),
			(longitudinal_segments - 1),
			make_torus_verts(longitudinal_segments, transverse_segments, hole_ratio, length, true).get(),
			make_torus_quad_strip_indices(longitudinal_segments - 1, transverse_segments, false).get()
		);
	else
		return new Model(
			GL_QUADS,
			longitudinal_segments * transverse_segments,
			4,
			(longitudinal_segments - 1) * transverse_segments,
			make_torus_verts(longitudinal_segments, transverse_segments, hole_ratio, length, true).get(),
			make_torus_quad_indices(longitudinal_segments - 1, transverse_segments, false).get()
		);
}


std::shared_ptr<Vec4[]> Model::s3ify(int count, double scale, const Vec3* vertices)
{
	std::shared_ptr<Vec4[]> ret(new Vec4[count]);
	for(int i = 0; i < count; i++)
	{
		/*
			This is the most straightforward way to project onto S3. We could also 
			get the magnitude and take its secent to preserve distance from the origin.
		*/
		const Vec3& temp = vertices[i];
		ret[i] = Vec4(
			temp.x * scale,
			temp.y * scale,
			temp.z * scale,
			1
		).normalize();
	}

	return ret;
}
