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


//extern const Vec4 xhat, yhat, zhat, what;

extern Mat4 s_cam_mat, s_light_mat;
extern Mat4 s_cam_projection, s_light_projection;

extern double s_screen_aspect_ratio, s_fog_scale;

extern const Mat4 s_cube_xforms[6];

void translate_cam(double right, double down, double fwd);
void rotate_cam(double pitch, double yaw, double roll);

//Sets proj_mat to the appropriate projection matrix. The far clipping plane will always be at tau.
//Also sets aspect_ratio, which is exposed as a global above just so that Model.cpp can feed it to shaders.
void set_perspective(double new_aspect_ratio, double vertical_field_of_view = TAU / 4, double near = 0.001);

//a and b are assumed to be normalized but not necessarily orthogonal.
//If chord is non-NULL, it will be filled in with the distance between a and b.
Mat4 basis_around(Vec4 a, Vec4 b, double *chord = NULL);
