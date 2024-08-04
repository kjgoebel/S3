#pragma once

#include "Camera.h"

#include "Model.h"


struct Light : public Camera
{
	Vec3 emission;
	Model* model;

	Light(Mat4& mat, Vec3& emission, Model* model, double near_clip = 0.001)
		: Camera(mat, 1, TAU / 4, near_clip), emission(emission), model(model) {}
};
