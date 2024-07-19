#pragma once

#include "Vector.h"


double frand();
double fsrand();
double current_time();

Vec4 rand_s3();
Vec3 rand_s2();

char* read_file(const char* filename);

void error(const char* fmt, ...);
