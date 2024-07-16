#include "S3.h"


const Vec4 xhat(1, 0, 0, 0), yhat(0, 1, 0, 0), zhat(0, 0, 1, 0), what(0, 0, 0, 1);

Mat4 cam_mat = Mat4::identity();

Mat4 proj_mat = Mat4::identity();

float aspect_ratio = 1;


/*
	Note that these implementations of translate_cam() and rotate_cam() are 
	biased because the rotation matrices generated don't commute.
*/
void translate_cam(double right, double down, double fwd)
{
	cam_mat = Mat4::axial_rotation(_w, _x, right) * Mat4::axial_rotation(_w, _y, down) * Mat4::axial_rotation(_z, _w, fwd) * cam_mat;
}

void rotate_cam(double pitch, double yaw, double roll)
{
	cam_mat = Mat4::axial_rotation(_y, _z, pitch) * Mat4::axial_rotation(_x, _z, yaw) * Mat4::axial_rotation(_y, _x, roll) * cam_mat;
}
