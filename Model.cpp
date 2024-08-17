#include "Model.h"

#include "MakeModel.h"
#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <memory>
#include "Utils.h"
#include "Framebuffer.h"

#pragma warning(disable : 4244)		//conversion from double to float

//#define VERIFY_BUFFERS
//#define VERIFY_BUFFER_ASSIGNMENT


Model::Model(int num_verts, const Vec4* verts, const Vec4* vert_colors)
{
	int i;
	primitive = GL_POINTS;
	num_vertices = num_verts;

	//This is to make the draw code simpler.
	num_primitives = 1;
	vertices_per_primitive = num_verts;

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
	elements = NULL;
	normals = NULL;

	vertex_buffer = vertex_color_buffer = element_buffer = normal_buffer = 0;
	raw_vertex_array = 0;
}

Model::Model(
	int prim,
	int num_verts,
	int verts_per_prim,
	int num_prims,
	const Vec4* verts,
	const unsigned int* ixes,
	const Vec4* vert_colors,
	const Vec4* norms
) {
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
	if(norms)
	{
		normals = std::unique_ptr<Vec4[]>(new Vec4[num_verts]);
		for(i = 0; i < num_verts; i++)
			normals[i] = norms[i];
	}
	else
		normals = NULL;

	vertex_buffer = vertex_color_buffer = element_buffer = normal_buffer = 0;
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

	if(element_buffer)
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);

	if(normal_buffer)
	{
		glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
		glVertexAttribPointer(2, 4, GL_DOUBLE, GL_FALSE, sizeof(Vec4), (void*)0);
		glEnableVertexAttribArray(2);
	}

	glBindVertexArray(0);

	return vertex_array;
}

void Model::prepare_to_render()
{
	if(vertex_buffer)
		error("Model was already prepared for rendering.\n");
		
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, num_vertices * sizeof(Vec4), vertices.get(), GL_STATIC_DRAW);
	#ifdef VERIFY_BUFFERS
		fprintf(stderr, "%d vertices = %d\n", num_vertices, vertex_buffer);
		for(int i = 0; i < num_vertices; i++)
		{
			fprintf(stderr, "\t");
			print_vector(vertices[i], stderr);
		}
	#endif

	if(vertex_colors)
	{
		glGenBuffers(1, &vertex_color_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_color_buffer);
		glBufferData(GL_ARRAY_BUFFER, num_vertices * sizeof(Vec4), vertex_colors.get(), GL_STATIC_DRAW);
		#ifdef VERIFY_BUFFERS
			fprintf(stderr, "%d vertex colors = %d\n", num_vertices, vertex_buffer);
			for(int i = 0; i < num_vertices; i++)
			{
				fprintf(stderr, "\t");
				print_vector(vertex_colors[i], stderr);
			}
		#endif
	}

	if(elements)
	{
		glGenBuffers(1, &element_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_primitives * vertices_per_primitive * sizeof(GLuint), elements.get(), GL_STATIC_DRAW);
		#ifdef VERIFY_BUFFERS
			fprintf(stderr, "%d x %d elements = %d\n", num_primitives, vertices_per_primitive, vertex_buffer);
			for(int i = 0; i < num_primitives; i++)
			{
				fprintf(stderr, "\t");
				for(int j = 0; j < vertices_per_primitive; j++)
					fprintf(stderr, "%d ", elements[i * vertices_per_primitive + j]);
				fprintf(stderr, "\n");
			}
		#endif
	}

	if(normals)
	{
		glGenBuffers(1, &normal_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
		glBufferData(GL_ARRAY_BUFFER, num_vertices * sizeof(Vec4), normals.get(), GL_STATIC_DRAW);
		#ifdef VERIFY_BUFFERS
			fprintf(stderr, "%d normals = %d\n", num_vertices, normal_buffer);
			for(int i = 0; i < num_vertices; i++)
			{
				fprintf(stderr, "\t");
				print_vector(normals[i], stderr);
			}
		#endif
	}

	raw_vertex_array = make_vertex_array();
}

ShaderProgram* Model::get_shader_program(bool shadow, bool instanced_xforms, bool instanced_base_colors)
{
	auto options = std::set<const char*>();
	if(vertex_colors)
		options.insert(DEFINE_VERTEX_COLOR);
	if(normals)
		options.insert(DEFINE_VERTEX_NORMAL);
	if(instanced_base_colors)
		options.insert(DEFINE_INSTANCED_BASE_COLOR);
	if(shadow)
		options.insert(DEFINE_SHADOW);
	auto vert_options = options;
	if(instanced_xforms)
		vert_options.insert(DEFINE_INSTANCED_XFORM);
	return ShaderProgram::get(
		Shader::get(vert, vert_options),
		Shader::get(primitive == GL_POINTS ? geom_points : geom_triangles, options),
		Shader::get(primitive == GL_POINTS ? frag_points : frag, options)
	);
}


void Model::draw(const Mat4& xform, const Vec4& base_color)
{
	if(!vertex_buffer)
		prepare_to_render();
		
	ShaderProgram* raw_program = get_shader_program(s_is_shadow_pass(), false, false);
	raw_program->use();
	raw_program->set_vector("base_color", base_color);
	raw_program->set_matrix("model_view_xform", ~s_curcam->get_mat() * xform);		//That should be the inverse of cam_mat, but it _should_ always be SO(4), so the inverse _should_ always be the transpose....
	
	glBindVertexArray(raw_vertex_array);
	draw_raw();
	glBindVertexArray(0);
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
		glEnableVertexAttribArray(3 + i);
		glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)(4 * i * sizeof(float)));
		glVertexAttribDivisor(3 + i, 1);
	}

	glBindVertexArray(0);
}

void Model::bind_color_array(GLuint vertex_array, int count, const Vec4* base_colors)
{
	glBindVertexArray(vertex_array);

	GLuint base_color_buffer;
	glGenBuffers(1, &base_color_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, base_color_buffer);
	glBufferData(GL_ARRAY_BUFFER, count * sizeof(Vec4), base_colors, GL_STATIC_DRAW);
	glEnableVertexAttribArray(7);
	glVertexAttribPointer(7, 4, GL_DOUBLE, GL_FALSE, sizeof(Vec4), (void*)0);
	glVertexAttribDivisor(7, 1);

	glBindVertexArray(0);
}

void Model::draw_raw()
{
	#ifdef VERIFY_BUFFER_ASSIGNMENT
		printf("%d, %d, %d, %d, %d\n", num_vertices, raw_vertex_array, vertex_buffer, vertex_color_buffer, element_buffer);
		GLint temp;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &temp);
		printf("\t%d\n", temp);
		for(int i = 0; i < 7; i++)
		{
			glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &temp);
			printf("\t\t%d: %d", i, temp);
			glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &temp);
			printf(" %d\n", temp);
		}
	#endif

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
					glDrawElements(primitive, vertices_per_primitive, GL_UNSIGNED_INT, (void*)(i * vertices_per_primitive * sizeof(GLuint)));
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
				for(int i = 0; i < num_primitives; i++)
					glDrawElementsInstanced(primitive, vertices_per_primitive, GL_UNSIGNED_INT, (void*)(i * vertices_per_primitive * sizeof(GLuint)), count);
				break;
		}
	else
		for(int i = 0; i < num_primitives; i++)
			glDrawArraysInstanced(primitive, i * vertices_per_primitive, vertices_per_primitive, count);
}


DrawFunc Model::make_draw_func(int count, const Mat4* xforms, Vec4 base_color, bool use_instancing)
{
	if(!vertex_buffer)
		prepare_to_render();

	if(use_instancing)
	{
		GLuint vertex_array = make_vertex_array();
		bind_xform_array(vertex_array, count, xforms);

		return [count, vertex_array, base_color, this]() {
			ShaderProgram* program = get_shader_program(s_is_shadow_pass(), true, false);
			program->use();
			glBindVertexArray(vertex_array);
			program->set_vector("base_color", base_color);
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
			ShaderProgram* program = get_shader_program(s_is_shadow_pass(), false, false);
			program->use();
			program->set_vector("base_color", base_color);
			glBindVertexArray(raw_vertex_array);
			for(int i = 0; i < count; i++)
			{
				program->set_matrix("model_view_xform", ~s_curcam->get_mat() * temp_xforms[i]);
				draw_raw();
			}
			glBindVertexArray(0);
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
		bind_color_array(vertex_array, count, base_colors);

		return [count, vertex_array, this]() {
			ShaderProgram* program = get_shader_program(s_is_shadow_pass(), true, true);
			program->use();
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
			ShaderProgram* program = get_shader_program(s_is_shadow_pass(), false, false);
			program->use();
			glBindVertexArray(raw_vertex_array);
			for(int i = 0; i < count; i++)
			{
				Vec4 base_color = temp_colors[i];
				program->set_vector("base_color", base_color);
				program->set_matrix("model_view_xform", ~s_curcam->get_mat() * temp_xforms[i]);
				draw_raw();
			}
			glBindVertexArray(0);
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
	if(!elements)
		error("Model has no primitives to color.");

	std::unique_ptr<Vec4[]> old_verts = std::move(vertices);
	num_vertices = num_primitives * vertices_per_primitive;
	vertices.reset(new Vec4[num_vertices]);

	std::unique_ptr<GLuint[]> old_elements = std::move(elements);
	elements = NULL;

	std::unique_ptr<Vec4[]> old_normals;
	if(normals)
	{
		old_normals = std::move(normals);
		normals.reset(new Vec4[num_vertices]);
	}

	vertex_colors = std::unique_ptr<Vec4[]>(new Vec4[num_vertices]);

	for(int i = 0; i < num_primitives; i++)
	{
		Vec4 temp(scale * frand(), scale * frand(), scale * frand(), 0);
		for(int j = 0; j < vertices_per_primitive; j++)
		{
			int ix = vertices_per_primitive * i + j;
			vertices[ix] = old_verts[old_elements[ix]];
			vertex_colors[ix] = temp;
			if(normals)
				normals[ix] = old_normals[old_elements[ix]];
		}
	}
}


typedef std::function<void(int, int, int)> TriangleFunc;

/*
	Convert a primitive into triangles. Except it's indirect, so convert a primitive into the 
	indices into the elements list corresponding to those triangles. It's indirect because 
	Models may or may not use element lists, so the actual list being indexed might be 
	elements or vertices.
*/
void _split_into_triangles_indirect(int primitive, int start, int verts_per_primitive, TriangleFunc emit_triangle)
{
	switch(primitive)
	{
		case GL_TRIANGLES:
			//Ooh, I know this one!
			if(verts_per_primitive != 3)
				error("Primitive is triangles but verts_per_primitive = %d\n", verts_per_primitive);
			emit_triangle(start, start + 1, start + 2);
			break;
		case GL_QUADS:
			if(verts_per_primitive != 4)
				error("Primitive is quads but verts_per_primitive = %d\n", verts_per_primitive);
			emit_triangle(start, start + 1, start + 2);
			emit_triangle(start, start + 2, start + 3);
			break;
		case GL_QUAD_STRIP:
			for(int base = start; base < start + verts_per_primitive - 2; base += 2)
			{
				emit_triangle(base, base + 2, base + 1);
				emit_triangle(base + 1, base + 2, base + 3);
			}
			break;
		default:
			error("Don't know how to split primitive type %d into triangles.\n");
			break;
	}
}

void Model::generate_normals()
{
	normals = std::unique_ptr<Vec4[]>(new Vec4[num_vertices]);
	for(int i = 0; i < num_vertices; i++)
		normals[i] = Vec4(0, 0, 0, 0);

	std::shared_ptr<int[]> times_touched(new int[num_vertices]);

	for(int prim = 0; prim < num_primitives; prim++)
	{
		_split_into_triangles_indirect(
			primitive,
			prim * vertices_per_primitive,
			vertices_per_primitive,
			[this, times_touched](int a, int b, int c) {
				if(elements)
				{
					a = elements[a];
					b = elements[b];
					c = elements[c];
				}
				Vec4 va = vertices[a], vb = vertices[b], vc = vertices[c];

				/*
					You might think vb and vc are close enough to va that the tangents 
					don't need to be orthogonalized to va (since the final normals are 
					going to be orthonormalized against vb and vc anyway). But no, the 
					~0.02 dot product between the tangents and va occasionally 
					derails the normal generation catastrophically.
				*/
				Vec4 tangent = vb - va;
				tangent = (tangent - va * (va * tangent)).normalize();
				Vec4 bitangent = vc - va;
				bitangent = (bitangent - tangent * (tangent * bitangent) - va * (va * bitangent)).normalize();

				Vec4 temp;
				do {
					temp = rand_s3();
				}while((temp - tangent).mag2() < 0.2 || (temp - bitangent).mag2() < 0.2 || (temp - va).mag2() < 0.2);
				//In theory we should check against vertices b and c as well, but we assume they're close to a.

				//an approximate normal to the triangle. We check for handedness only once because vb and vc are close to va.
				temp = temp - tangent * (tangent * temp) - bitangent * (bitangent * temp) - va * (va * temp);
				Mat4 check = Mat4::from_columns(tangent, bitangent, temp, va);
				if(check.determinant() > 0)		//Why is this >?
					temp = -temp;

				//We have to orthonormalize temp three times because va, vb and vc are not parallel. Yes, there's 
				//no such thing as a flat triangle in a curved space.
				normals[a] = normals[a] + temp.normalize();
				times_touched[a]++;

				normals[b] = normals[b] + (temp - vb * (vb * temp)).normalize();
				times_touched[b]++;

				normals[c] = normals[c] + (temp - vc * (vc * temp)).normalize();
				times_touched[c]++;
			}
		);
	}

	for(int i = 0; i < num_vertices; i++)
		normals[i] = (normals[i] / times_touched[i]).normalize();
}


Model* Model::make_icosahedron(double scale, int subdivisions, bool normalize) {
	std::unique_ptr<TriangleModel> ico(new TriangleModel(12, 20, icosahedron_verts, icosahedron_elements));

	for(int i = 0; i < subdivisions; i++)
		ico->subdivide(normalize);

	return new Model(
		GL_TRIANGLES,
		ico->get_num_vertices(),
		3,
		ico->get_num_triangles(),
		s3ify(ico->get_num_vertices(), scale, ico->get_vertices()).get(),
		ico->get_elements()
	);
}

Model* Model::make_torus(int longitudinal_segments, int transverse_segments, double hole_ratio, bool use_quad_strips, bool make_normals)
{
	if(use_quad_strips)
		return new Model(
			GL_QUAD_STRIP,
			longitudinal_segments * transverse_segments,
			2 * (transverse_segments + 1),
			longitudinal_segments,
			make_torus_verts(longitudinal_segments, transverse_segments, hole_ratio).get(),
			make_torus_quad_strip_indices(longitudinal_segments, transverse_segments).get(),
			NULL,
			make_normals ? make_torus_normals(longitudinal_segments, transverse_segments, hole_ratio).get() : NULL
		);
	else
		return new Model(
			GL_QUADS,
			longitudinal_segments * transverse_segments,
			4,
			longitudinal_segments * transverse_segments,
			make_torus_verts(longitudinal_segments, transverse_segments, hole_ratio).get(),
			make_torus_quad_indices(longitudinal_segments, transverse_segments).get(),
			NULL,
			make_normals ? make_torus_normals(longitudinal_segments, transverse_segments, hole_ratio).get() : NULL
		);
}


Model* Model::make_torus_arc(int longitudinal_segments, int transverse_segments, double length, double hole_ratio, bool use_quad_strips, bool make_normals)
{
	if(use_quad_strips)
		return new Model(
			GL_QUAD_STRIP,
			longitudinal_segments * transverse_segments,
			2 * (transverse_segments + 1),
			(longitudinal_segments - 1),
			make_torus_verts(longitudinal_segments, transverse_segments, hole_ratio, length, true).get(),
			make_torus_quad_strip_indices(longitudinal_segments - 1, transverse_segments, false).get(),
			NULL,
			make_normals ? make_torus_normals(longitudinal_segments, transverse_segments, hole_ratio).get() : NULL
		);
	else
		return new Model(
			GL_QUADS,
			longitudinal_segments * transverse_segments,
			4,
			(longitudinal_segments - 1) * transverse_segments,
			make_torus_verts(longitudinal_segments, transverse_segments, hole_ratio, length, true).get(),
			make_torus_quad_indices(longitudinal_segments - 1, transverse_segments, false).get(),
			NULL,
			make_normals ? make_torus_normals(longitudinal_segments, transverse_segments, hole_ratio).get() : NULL
		);
}


Model* Model::make_bumpy_torus(int longitudinal_segments, int transverse_segments, double bump_height, bool use_quad_strips)
{
	if(use_quad_strips)
		return new Model(
			GL_QUAD_STRIP,
			longitudinal_segments * transverse_segments,
			2 * (transverse_segments + 1),
			longitudinal_segments,
			make_bumpy_torus_verts(longitudinal_segments, transverse_segments, bump_height).get(),
			make_torus_quad_strip_indices(longitudinal_segments, transverse_segments).get()
		);
	else
		return new Model(
			GL_QUADS,
			longitudinal_segments * transverse_segments,
			4,
			longitudinal_segments * transverse_segments,
			make_bumpy_torus_verts(longitudinal_segments, transverse_segments, bump_height).get(),
			make_torus_quad_indices(longitudinal_segments, transverse_segments).get()
		);
}


std::shared_ptr<Vec4[]> Model::s3ify(int count, double scale, const Vec3* vertices)
{
	std::shared_ptr<Vec4[]> ret(new Vec4[count]);
	for(int i = 0; i < count; i++)
	{
		/*
			This is the most straightforward way to project onto S3. We could also 
			get the magnitude and take its secant to preserve distance from the origin.
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
