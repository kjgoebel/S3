#pragma once

#include "Camera.h"

#include "Model.h"


#define LIGHT_MAP_SIZE	(4096)


struct Light : public Camera
{
	Vec3 emission;
	Model* model;

	Light(Mat4& mat, Vec3& emission, Model* model, double near_clip = 0.001)
		: Camera(mat, 1, TAU / 4, near_clip), emission(emission), model(model) {}

	void render(DrawFunc draw_scene);		//Render the light map with draw_scene(), and then draw the light's effect into abuffer.
	void draw();							//Draw the light's model.
};
