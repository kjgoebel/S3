#pragma once


#include "Vector.h"


Vec4 torus_world_xform(Vec3 p);
Mat4 torus_world_xform(Vec3 p, double yaw, double pitch, double roll);

Vec3 inverse_torus_world_xform(Vec4 p);

bool cast_ray(Vec4 p, Vec4 v, Vec4& out_hit);

Vec3 random_torus_pos(double min_height, double max_height);
