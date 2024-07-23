#include "S3.h"
#include "Utils.h"


//const Vec4 xhat(1, 0, 0, 0), yhat(0, 1, 0, 0), zhat(0, 0, 1, 0), what(0, 0, 0, 1);

Mat4 cam_mat = Mat4::identity();

Mat4 proj_mat = Mat4::identity();

double aspect_ratio = 1;
double fog_scale = 1.5;


/*
	Note that these implementations of translate_cam() and rotate_cam() are 
	biased because the rotation matrices generated don't commute.
*/
void translate_cam(double right, double down, double fwd)
{
	cam_mat = cam_mat * Mat4::axial_rotation(_w, _x, right) * Mat4::axial_rotation(_w, _y, down) * Mat4::axial_rotation(_w, _z, fwd);
}

void rotate_cam(double pitch, double yaw, double roll)
{
	cam_mat = cam_mat * Mat4::axial_rotation(_y, _z, pitch) * Mat4::axial_rotation(_z, _x, yaw) * Mat4::axial_rotation(_x, _y, roll);
}


#define FAR		(TAU)

void set_perspective(double ar, double vfov, double near)
{
	aspect_ratio = ar;
	double q = tan(0.5 * vfov);

	proj_mat = Mat4(
		1.0 / (ar * q),		0,				0,								0,
		0,					-1.0 / q,		0,								0,
		0,					0,				(FAR + near) / (FAR - near),	2.0 * near * FAR / (near - FAR),
		0,					0,				1,								0
	);
}


//#include <stdio.h>

Mat4 basis_around(Vec4 a, Vec4 b, double* length)
{
	/*printf("a, b:\n");
	printf("\t"); print_vector(a);
	printf("\t"); print_vector(b);*/

	double dp = a * b;
	//printf("length = %f\n", acos(dp));
	if(length)
		*length = acos(dp);
	b = (b - dp * a).normalize();

	/*printf("Corrected a, b:\n");
	printf("\t"); print_vector(a);
	printf("\t"); print_vector(b);*/

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
		
	/*printf("ret =\n");
	print_matrix(ret);
	printf("whose determinant is %s.n\n", ret.determinant() > 0 ? "positive" : "negative");*/

	if(ret.determinant() < 0)
		ret.set_column(0, -temp1);
		
	/*printf("ret =\n");
	print_matrix(ret);
	printf("whose determinant is %s.n\n", ret.determinant() > 0 ? "positive" : "negative");*/

	return ret;
}
