#include "Utils.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include "GL/glew.h"

#pragma warning(disable : 4244)



//RNG adapted from Numerical Recipes 3rd Edition (Generator A1).

#define A1	(21)
#define A2	(35)
#define A3	(4)

unsigned long long random_seed;

void init_random()
{
	random_seed = clock();
}

unsigned long long random()
{
	random_seed ^= (random_seed >> A1);
	random_seed ^= (random_seed << A2);
	random_seed ^= (random_seed >> A3);
	return random_seed;
}

double frand()
{
	return 5.42101086242752217E-20 * random();
}

double fsrand()
{
	return 2.0 * frand() - 1.0;
}

double current_time()
{
	return (double)clock() / CLOCKS_PER_SEC;
}


Vec4 rand_s3()
{
	for(;;)
	{
		Vec4 ret(fsrand(), fsrand(), fsrand(), fsrand());
		float l2 = ret.mag2();
		if(l2 > 0 && l2 < 1)
		{
			ret.normalize_in_place();
			return ret;
		}
	}
}

Vec3 rand_s2()
{
	for(;;)
	{
		Vec3 ret(fsrand(), fsrand(), fsrand());
		float l2 = ret.mag2();
		if(l2 > 0 && l2 < 1)
		{
			ret.normalize_in_place();
			return ret;
		}
	}
}


void error(const char* fmt, ...)
{
	va_list arg_list;
	char buffer[4096];

	va_start(arg_list, fmt);
	vsprintf(buffer, fmt, arg_list);
	fprintf(stderr, buffer);
	va_end(arg_list);
	exit(-1);
}


void check_gl_errors(const char* check_point_name)
{
	GLenum error = glGetError();
	if(error != GL_NO_ERROR)
		fprintf(stderr, "GL Error %d: %s: %s\n", error, check_point_name, gluErrorString(error));
}