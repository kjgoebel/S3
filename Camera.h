#pragma once

#include "Vector.h"

/*
	How does it work?

	I use R4 coordinates everywhere on the CPU, and only convert to 3D coordinates 
	on the GPU. Object location/orientation is stored as a 4x4 matrix, which is 
	constrained to be SO(4). All transforms are SO(4) matrices. The W column is 
	the object's position, so transformations that involve W are "translations" 
	and transformations that don't are "rotations".

	My solution to the eternal coordinate headache is to reverse the Y axis. So, 
	+X is right, +Y is down, +Z is forward and +W is position. The projection 
	matrix inverts Y but not Z.

	cam_mat stores the matrix of the view point as an object in the world, so its 
	inverse is what should be used in rendering.
*/


class Camera
{
	Mat4 mat;
	double aspect_ratio;
	Mat4 projection;

public:
	Camera(Mat4& mat = Mat4::identity(), double aspect_ratio = 1, double vertical_field_of_view = TAU / 4);

	void set_mat(Mat4& new_mat) {mat = new_mat;}

	//Far clipping plane will always be at TAU.
	void set_perspective(double new_aspect_ratio, double vertical_field_of_view = TAU / 4);

	void translate(double right, double down, double fwd);
	void rotate(double pitch, double yaw, double roll);

	const Mat4& get_mat() const {return mat;}
	double get_aspect_ratio() const {return aspect_ratio;}
	const Mat4& get_proj() const {return projection;}
};


extern Camera cam;

extern double s_fog_scale;
extern const Mat4 s_cube_xforms[6];

extern Camera* s_curcam;

extern double near_clipping_plane;		//This needs to be the same for all cameras and lights.
