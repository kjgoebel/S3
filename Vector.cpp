#include "Vector.h"
#include <stdio.h>


void print_vector(const Vec4& v, FILE* fout)
{
	for(int i = 0; i < 4; i++)
		fprintf(fout, "% 0.4f ", v[i]);
	fprintf(fout, "\n");
}

void print_matrix(const Mat4& m, FILE* fout)
{
	for(int i = 0; i < 4; i++)
	{
		for(int j = 0; j < 4; j++)
			fprintf(fout, "% 0.4f ", m.data[i][j]);
		fprintf(fout, "\n");
	}
}