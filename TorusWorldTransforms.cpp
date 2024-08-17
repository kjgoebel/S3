#include "TorusWorldTransforms.h"

#include <functional>
#include "Utils.h"


Vec4 torus_world_xform(Vec3 p)
{
	double cx = cos(p.x), sx = sin(p.x), cy = cos(p.y), sy = sin(p.y);
	return INV_ROOT_2 * Vec4(cx, -sx, cy, sy);
}

Mat4 torus_world_xform(Vec3 p, double yaw, double pitch, double roll)
{
	double cx = cos(p.x), sx = sin(p.x), cy = cos(p.y), sy = sin(p.y);
	Vec4 pos = INV_ROOT_2 * Vec4(cx, -sx, cy, sy);
	Vec4 fwd = Vec4(0, 0, -sy, cy);

	Vec4 down = INV_ROOT_2 * Vec4(-cx, sx, cy, sy);
	Vec4 right = Vec4(-sx, -cx, 0, 0);
	
	return Mat4::from_columns(right, down, fwd, pos)
				* Mat4::axial_rotation(_y, _w, p.z)
				* Mat4::axial_rotation(_z, _x, yaw)
				* Mat4::axial_rotation(_y, _z, pitch)
				* Mat4::axial_rotation(_x, _y, roll);
}


Vec3 inverse_torus_world_xform(Vec4 p)
{
	Vec3 ret;
	ret.x = atan2(-p.y, p.x);
	ret.y = atan2(p.w, p.z);
	double	sz = sqrt(1 - p.w * p.w - p.z * p.z),
			cz = sqrt(1 - p.y * p.y - p.x * p.x);
	ret.z = atan2(sz, cz) - TAU / 8;
	return ret;
}


typedef std::function<double(double)> SearchableFunc;

bool r_binary_search(SearchableFunc f, double x0, double x1, double f0, double f1, double tolerance, double& result)
{
	if(f0 > 0 == f1 > 0)
		return false;
	double xc = 0.5 * (x0 + x1), fc = f(xc);
	if(x1 - x0 < tolerance)
	{
		result = xc;
		return true;
	}
	if(fc > 0 == f1 > 0)
		return r_binary_search(f, x0, xc, f0, fc, tolerance, result);
	else
		return r_binary_search(f, xc, x1, fc, f1, tolerance, result);
}

//Find intersection between the ray p, v and the Torus.
//Dunno how to solve this exactly, so use binary search.
bool cast_ray(Vec4 p, Vec4 v, Vec4& out_hit)
{
	double	c2 = p.x * p.x + p.y * p.y - p.z * p.z - p.w * p.w,
			s2 = v.x * v.x + v.y * v.y - v.z * v.z - v.w * v.w,
			sc = 2 * (p.x * v.x + p.y * v.y - p.z * v.z - p.w * v.w),
			alpha;

	SearchableFunc func = [c2, s2, sc] (double a) {
		double c = cos(a), s = sin(a);
		return c2 * c * c + s2 * s * s + sc * s * c;
	};

	#define NUM_RAY_SEGMENTS (5)
	for(int i = 0; i < NUM_RAY_SEGMENTS; i++)
	{
		double start = TAU / NUM_RAY_SEGMENTS * i;
		double end = start + TAU / NUM_RAY_SEGMENTS;
		if(r_binary_search(func, start, end, func(start), func(end), 0.01, alpha))
		{
			out_hit = cos(alpha) * p + sin(alpha) * v;
			return true;
		}
	}
	return false;
}


Vec3 random_torus_pos(double min_height, double max_height)
{
	return Vec3(frand() * TAU, frand() * TAU, min_height + frand() * (max_height - min_height));
}
