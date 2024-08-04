#include "Vector.h"
#include <stdio.h>
#include "Utils.h"


//const Vec4 xhat(1, 0, 0, 0), yhat(0, 1, 0, 0), zhat(0, 0, 1, 0), what(0, 0, 0, 1);

Mat4 basis_around(Vec4 a, Vec4 b, double* chord)
{
	double dp = a * b;
	if(chord)
		*chord = acos(dp);
	b = (b - dp * a).normalize();

	Vec4 temp1;
	do {
		temp1 = rand_s3();
	}while((temp1 - a).mag2() < 0.2 || (temp1 - b).mag2() < 0.2);
	temp1 = (temp1 - a * (a * temp1) - b * (b * temp1)).normalize();

	Vec4 temp2;
	do {
		temp2 = rand_s3();
	}while((temp2 - a).mag2() < 0.2 || (temp2 - b).mag2() < 0.2 || (temp2 - temp1).mag2() < 0.2);
	temp2 = (temp2 - a * (a * temp2) - b * (b * temp2) - temp1 * (temp1 * temp2)).normalize();

	Mat4 ret = Mat4::from_columns(temp1, temp2, b, a);

	if(ret.determinant() < 0)
		ret.set_column(0, -temp1);

	return ret;
}

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