#pragma once

#include "Vector.h"


//extern const Vec4 xhat, yhat, zhat, what;

extern Mat4 cam_mat;
extern Mat4 proj_mat;

extern float aspect_ratio, fog_scale;

void translate_cam(double right, double down, double fwd);
void rotate_cam(double pitch, double yaw, double roll);

//Sets proj_mat to the appropriate projection matrix. The far clipping plane will always be at tau.
//Also sets aspect_ratio, which is exposed just so that it can be fed to shaders.
void set_perspective(double aspect_ratio, double vertical_field_of_view = TAU / 4, double near = 0.001);
