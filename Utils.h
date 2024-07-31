#pragma once

#include "Vector.h"


double frand();
double fsrand();
double current_time();

Vec4 rand_s3();
Vec3 rand_s2();

void error(const char* fmt, ...);

void check_gl_errors(const char* check_point_name);
