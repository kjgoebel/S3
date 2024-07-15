#include "Vector.h"
#include <stdio.h>


void print_vector(const Vec4& v)
{
	for(int i = 0; i < 4; i++)
		printf("% 0.4f ", v[i]);
	printf("\n");
}

void print_matrix(const Mat4& m)
{
	for(int i = 0; i < 4; i++)
	{
		for(int j = 0; j < 4; j++)
			printf("% 0.4f ", m.data[i][j]);
		printf("\n");
	}
}