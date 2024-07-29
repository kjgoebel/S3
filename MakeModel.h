#pragma once

#include "Vector.h"
#include <memory>
#include "GL/glew.h"


/*
	Generate vertex/normal lists for a surface like normalization_factor * (sin(phi), cos(phi), sin(theta), cos(theta)).
	The phi parts are multiplied by hole_ratio, so hole_ratio is the ratio of the size of the what-zhat hole to the size of the xhat-yhat hole.
	length is the maximum value of theta.
	So hole_ratio = 1, length = TAU gives you the Clifford torus.
	If make_final_ring is true, the rings will end at theta = TAU, rather than leaving room for the loop back to the first ring.
	      | theta = length
	:  :  : make_final_ring = true
	: : :   make_final_ring = false
	Number of vertices/normals generated will be long_segments * 2 * (trans_segments + 1).
*/
std::shared_ptr<Vec4[]> make_torus_verts(int long_segments, int trans_segments, double hole_ratio, double length = TAU, bool make_final_ring = false);
std::shared_ptr<Vec4[]> make_torus_normals(int long_segments, int trans_segments, double hole_ratio, double length = TAU, bool make_final_ring = false);

//Assumes hole_ratio = 1, length = TAU and make_final_ring = false.
std::shared_ptr<Vec4[]> make_bumpy_torus_verts(int long_segments, int trans_segments, double bump_height);

/*
	Generate elements for the above torus. loop_longitudinally = false corresponds to make_final_ring = true (and normally length < TAU).
	Quad strip version generates long_segments * 2 * (trans_segments + 1) elements.
	Quad version generates 4 * long_segments * trans_segments elements.
*/
std::shared_ptr<GLuint[]> make_torus_quad_strip_indices(int long_segments, int trans_segments, bool loop_longitudinally = true);
std::shared_ptr<GLuint[]> make_torus_quad_indices(int long_segments, int trans_segments, bool loop_longitudinally = true);
