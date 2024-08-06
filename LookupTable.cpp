#include "LookupTable.h"

#include <math.h>
#include "Vector.h"
#include "Utils.h"


#define CHORD2_LUT_SIZE		(64)

LookupTable* s_chord2_lut;


void init_luts()
{
	float* chord2_data = new float[2 * CHORD2_LUT_SIZE];
	for(int i = 0; i < CHORD2_LUT_SIZE; i++)
	{
		float c = 4.0 * i / (CHORD2_LUT_SIZE - 1);
		float distance = 2.0 * asin(0.5 * sqrt(c));
		chord2_data[2 * i] = distance / TAU;
		float temp = sin(2.0 * asin(0.5 * sqrt(c)));
		chord2_data[2 * i + 1] = 1.0 / (temp * temp);
	}
	//1 / sin^2(0) is infinite and my GPU doesn't seem to like infinity.
	//FLT_MAX is also too big, because it makes for a visible discontinuity at distance = 2 / (CHORD2_LUT_SIZE - 1).
	//Could try to model the actual size of point lights.
	chord2_data[1] = 3 * chord2_data[3];
	chord2_data[2 * CHORD2_LUT_SIZE - 1] = chord2_data[1];
		
	for(int i = 0; i < CHORD2_LUT_SIZE; i++)
	{
		float c = 4.0 * i / (CHORD2_LUT_SIZE - 1);
		printf("%f: %f, %f\n", c, chord2_data[2 * i], chord2_data[2 * i + 1]);
	}

	s_chord2_lut = new LookupTable(GL_TEXTURE_1D, CHORD2_LUT_SIZE, GL_RG32F, GL_RG, chord2_data, 0, 4);
	check_gl_errors("chord squared LUT setup");

	delete[] chord2_data;
}