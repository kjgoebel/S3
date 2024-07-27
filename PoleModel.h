#pragma once

#include "Vector.h"
#include "Model.h"



#define NUM_POLE_VERTS		(162)
#define NUM_POLE_TRIANGLES	(320)

extern Vec3 pole_model_vertices[NUM_POLE_VERTS];
extern GLuint pole_model_elements[3 * NUM_POLE_TRIANGLES];
