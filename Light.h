#pragma once

#include "Camera.h"

#include "Model.h"
#include "Framebuffer.h"



struct Light : public Camera
{
	Vec3 emission;
	Model* model;

	Framebuffer* shadow_buffer;
	Pass *shadow_pass, *light_pass;
	bool shadow_map_dirty;
	bool use_fog;

	Light(Mat4& mat, Vec3& emission, Model* model, bool use_fog = false, double near_clip = 0.001);

	inline GLuint shadow_map() {return shadow_buffer->textures[0];}

	void set_mat(Mat4& new_mat)
	{
		Camera::set_mat(new_mat);
		shadow_map_dirty = true;
	}

	void set_perspective(double new_aspect_ratio, double vertical_field_of_view, double near = 0.001)
	{
		error("The aspect ratio and field of view for a camera must always be 1 and TAU / 4, respectively.");
	}

	void set_perspective(double near_clip)
	{
		Camera::set_perspective(1, TAU / 4, near_clip);
	}

	void translate(double right, double down, double fwd)
	{
		Camera::translate(right, down, fwd);
		shadow_map_dirty = true;
	}
	void rotate(double pitch, double yaw, double roll)
	{
		Camera::rotate(pitch, yaw, roll);
		shadow_map_dirty = true;
	}

	void render(DrawFunc draw_scene);		//Render the light map with draw_scene(), and then draw the light's effect into abuffer.
	void draw();							//Draw the light's model.
};
