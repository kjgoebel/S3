#include "MakeModel.h"

#include "Utils.h"
#include <map>


std::shared_ptr<Vec4[]> make_torus_verts(int long_segments, int trans_segments, double hole_ratio, double length, bool make_final_ring)
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

std::shared_ptr<Vec4[]> make_torus_normals(int long_segments, int trans_segments, double hole_ratio, double length, bool make_final_ring)
{
	std::shared_ptr<Vec4[]> ret(new Vec4[long_segments * trans_segments]);
	for(int i = 0; i < long_segments; i++)
	{
		double theta = (double)i * length / (make_final_ring ? long_segments - 1 : long_segments);
		for(int j = 0; j < trans_segments; j++)
		{
			double phi = (double)j * TAU / trans_segments;

			ret[i * trans_segments + j] = INV_ROOT_2 * Vec4(
				sin(phi),
				cos(phi),
				-sin(theta),
				-cos(theta)
			);
		}
	}

	return ret;
}

std::shared_ptr<Vec4[]> make_bumpy_torus_verts(int long_segments, int trans_segments, double bump_height)
{
	std::shared_ptr<Vec4[]> ret(new Vec4[long_segments * trans_segments]);
	for(int i = 0; i < long_segments; i++)
	{
		double theta = (double)i * TAU / long_segments;
		for(int j = 0; j < trans_segments; j++)
		{
			double phi = (double)j * TAU / trans_segments;

			Vec4 pos(
				sin(phi),
				cos(phi),
				sin(theta),
				cos(theta)
			);
			Vec4 up(
				sin(phi),
				cos(phi),
				-sin(theta),
				-cos(theta)
			);
			ret[i * trans_segments + j] = (pos + fsrand() * bump_height * up).normalize();
		}
	}

	return ret;
}

std::shared_ptr<GLuint[]> make_torus_quad_strip_indices(int long_segments, int trans_segments, bool loop_longitudinally)
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

std::shared_ptr<GLuint[]> make_torus_quad_indices(int long_segments, int trans_segments, bool loop_longitudinally)
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


TriangleModel::TriangleModel(GLuint num_verts, GLuint num_tris, const Vec3* verts, const GLuint* elems)
		: num_vertices(num_verts), num_triangles(num_tris), vertices(new Vec3[num_verts]), elements(new GLuint[3 * num_tris])
{
	for(GLuint vert = 0; vert < num_vertices; vert++)
		vertices[vert] = verts[vert];
	for(GLuint elem = 0; elem < 3 * num_triangles; elem++)
		elements[elem] = elems[elem];
}

//#define DUMMY_SUBDIVIDE

void TriangleModel::subdivide(bool normalize)
{
	GLuint new_num_verts = num_vertices + num_triangles * 3 / 2;
	GLuint new_num_tris = num_triangles * 4;
	Vec3* new_verts = new Vec3[new_num_verts];
	GLuint* new_elems = new GLuint[3 * new_num_tris];

	#ifdef DUMMY_SUBDIVIDE
		for(int v = 0; v < new_num_verts; v++)
			new_verts[v] = rand_s2();
		for(int e = 0; e < 3 * new_num_tris; e++)
			new_elems[e] = random_raw() % new_num_verts;
	#else
		for(int v = 0; v < num_vertices; v++)
		{
			new_verts[v] = vertices[v];
			if(normalize)
				new_verts[v].normalize_in_place();
		}

		GLuint next_vert_index = num_vertices;
		std::map<std::pair<GLuint, GLuint>, GLuint> edge_table;

		for(GLuint t = 0; t < num_triangles; t++)
		{
			GLuint old_vert_indices[3] = {
				elements[3 * t + 0],
				elements[3 * t + 1],
				elements[3 * t + 2]
			};
			GLuint new_vert_indices[3];

			for(GLuint vi = 0; vi < 3; vi++)
			{
				GLuint edge_start = old_vert_indices[vi], edge_end = old_vert_indices[(vi + 1) % 3];
				if(edge_start > edge_end)
				{
					GLuint temp = edge_start;
					edge_start = edge_end;
					edge_end = temp;
				}

				if(edge_table.count({edge_start, edge_end}))
					new_vert_indices[vi] = edge_table[{edge_start, edge_end}];
				else
				{
					if(next_vert_index >= new_num_verts)
						error("Out of vertices!\n");
					new_vert_indices[vi] = next_vert_index;
					new_verts[next_vert_index] = vertices[edge_start] + vertices[edge_end];
					if(normalize)
						new_verts[next_vert_index].normalize_in_place();
					else
						new_verts[next_vert_index] = 0.5 * new_verts[next_vert_index];
					edge_table[{edge_start, edge_end}] = next_vert_index;
					next_vert_index++;
				}
			}

			if(4 * 3 * t + 3 * 3 + 2 >= 3 * new_num_tris)
				error("Out of elements!\n");

			new_elems[4 * 3 * t + 3 * 0 + 0] = old_vert_indices[0];
			new_elems[4 * 3 * t + 3 * 0 + 1] = new_vert_indices[0];
			new_elems[4 * 3 * t + 3 * 0 + 2] = new_vert_indices[2];
		
			new_elems[4 * 3 * t + 3 * 1 + 0] = old_vert_indices[1];
			new_elems[4 * 3 * t + 3 * 1 + 1] = new_vert_indices[1];
			new_elems[4 * 3 * t + 3 * 1 + 2] = new_vert_indices[0];
		
			new_elems[4 * 3 * t + 3 * 2 + 0] = old_vert_indices[2];
			new_elems[4 * 3 * t + 3 * 2 + 1] = new_vert_indices[2];
			new_elems[4 * 3 * t + 3 * 2 + 2] = new_vert_indices[1];
		
			new_elems[4 * 3 * t + 3 * 3 + 0] = new_vert_indices[0];
			new_elems[4 * 3 * t + 3 * 3 + 1] = new_vert_indices[1];
			new_elems[4 * 3 * t + 3 * 3 + 2] = new_vert_indices[2];
		}
	#endif

	num_vertices = new_num_verts;
	num_triangles = new_num_tris;
	vertices.reset(new_verts);
	elements.reset(new_elems);
}



#define A	(0.525731112119133606)
#define B	(0.850650808352039932)

const Vec3 icosahedron_verts[20] = {
	{-A, 0, B},
	{A, 0, B},
	{-A, 0, -B},
	{A, 0, -B},
	{0, B, A},
	{0, B, -A},
	{0, -B, A},
	{0, -B, -A},
	{B, A, 0},
	{-B, A, 0},
	{B, -A, 0},
	{-B, -A, 0}
};
const GLuint icosahedron_elements[3 * 20] = {
	1, 4, 0,
	4, 9, 0,
	4, 5, 9,
	8, 5, 4,
	1, 8, 4,
	1, 10, 8,
	10, 3, 8,
	8, 3, 5,
	3, 2, 5,
	3, 7, 2,
	3, 10, 7,
	10, 6, 7,
	6, 11, 7,
	6, 0, 11,
	6, 1, 0,
	10, 1, 6,
	11, 0, 9,
	2, 11, 9,
	5, 2, 9,
	11, 2, 7
};
