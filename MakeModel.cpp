#include "MakeModel.h"

#include "Utils.h"


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
