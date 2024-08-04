#include "Camera.h"
#include "Utils.h"


Camera cam;

Camera *s_curcam = &cam;

double s_fog_scale = 1.5;


Camera::Camera(const Mat4& mat, double aspect_ratio, double vertical_field_of_view, double near)
{
	this->mat = mat;
	set_perspective(aspect_ratio, vertical_field_of_view, near);
}


/*
	Note that these implementations of translate_cam() and rotate_cam() are 
	biased because the rotation matrices generated don't commute.
*/
void Camera::translate(double right, double down, double fwd)
{
	mat = mat * Mat4::axial_rotation(_w, _x, right) * Mat4::axial_rotation(_w, _y, down) * Mat4::axial_rotation(_w, _z, fwd);
}

void Camera::rotate(double pitch, double yaw, double roll)
{
	mat = mat * Mat4::axial_rotation(_y, _z, pitch) * Mat4::axial_rotation(_z, _x, yaw) * Mat4::axial_rotation(_x, _y, roll);
}


#define FAR		(TAU)

void Camera::set_perspective(double ar, double vfov, double near)
{
	aspect_ratio = ar;
	double q = tan(0.5 * vfov);

	projection = Mat4(
		1.0 / (ar * q),		0,				0,								0,
		0,					-1.0 / q,		0,								0,
		0,					0,				(FAR + near) / (FAR - near),	2.0 * near * FAR / (near - FAR),
		0,					0,				1,								0
	);
}


const Mat4 s_cube_xforms[6] = {
	Mat4::axial_rotation(_x, _z, TAU / 4),		//+X
	Mat4::axial_rotation(_z, _x, TAU / 4),		//-X
	Mat4::axial_rotation(_y, _z, TAU / 4),		//+Y
	Mat4::axial_rotation(_z, _y, TAU / 4),		//-Y
	Mat4::identity(),							//+Z
	Mat4::axial_rotation(_z, _x, TAU / 2)		//-Z
};
