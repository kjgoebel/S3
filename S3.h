#pragma once

#include "Vector.h"


extern const Vec4 xhat, yhat, zhat, what;

extern Mat4 cam_mat;
extern Mat4 proj_mat;

extern float aspect_ratio, fog_scale;

void translate_cam(double right, double down, double fwd);
void rotate_cam(double pitch, double yaw, double roll);
