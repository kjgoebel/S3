#pragma once

#include "GL/glew.h"
#include "Utils.h"
#include <vector>

#pragma warning(disable : 4244)		//conversion from double to float


extern int window_width, window_height;


struct TextureSpec
{
	GLenum target, internal_format, format, type, attachment_point, min_filter, mag_filter, wrap_mode;

	TextureSpec(
		GLenum target,
		GLenum internal_format,
		GLenum format,
		GLenum type,
		GLenum attachment_point,
		GLenum min_filter = GL_NEAREST,
		GLenum mag_filter = GL_NEAREST,
		GLenum wrap_mode = GL_CLAMP_TO_EDGE
	);

	void make_texture(GLuint* out_tex_name, GLuint framebuffer, GLsizei width, GLsizei height);
	
	//There really isn't a good name for this, 'cuz all it does is call glTexImage*(), and that's not a good name.
	void tex_image(GLuint tex_name, GLsizei width, GLsizei height, void* data = NULL);
};


//A framebuffer with no attachments. Don't use this directly.
struct Framebuffer
{
	GLuint name;
	GLsizei width, height;

	Framebuffer();
};

//A framebuffer whose attachments are screen resolution. Don't use this directly, except to call resize_all.
struct Screenbuffer : public Framebuffer
{
	virtual void post_resize()
	{
		width = window_width;
		height = window_height;
	}
	Screenbuffer() {all_screenbuffers.push_back(this);}
	static std::vector<Screenbuffer*> all_screenbuffers;
	static void post_resize_all();
};

//A G-buffer with albedo, position, normal and depth buffers in 32F formats.
struct GBuffer : public Screenbuffer
{
	GLuint albedo, position, normal, depth;

	GBuffer();

	void post_resize();

	static TextureSpec albedo_spec, position_spec, normal_spec, depth_spec;
};
//It is left up to the app (S3 or TorusWorld) to actually create this if needed:
extern GBuffer* s_gbuffer;

//A for Accumulation. This is what lights should draw into.
/*
	It has one color attachment, and also attaches the depth buffer from 
	a GBuffer so that non-lit (or even transparent) objects can be drawn 
	into it after the lighting passes.
*/
struct ABuffer : public Screenbuffer
{
	GLuint color;

	ABuffer(GBuffer* gbuffer);

	void post_resize();

	static TextureSpec color_spec;
};
extern ABuffer* s_abuffer;
/*
	Main.cpp doesn't use an A-Buffer, so this will just stay NULL in that app. 
	Shaders refer to it, but S3/Main doesn't use those shaders.
*/



struct Pass
{
	Framebuffer* framebuffer;		//NULL means default framebuffer.

	GLbitfield clear_mask;			//default color and depth
	bool depth_test, depth_mask;	//defaults true and true
	GLenum cull_face;				//0 means disable face culling. Default GL_BACK
	//So far, the only blending we need is GL_FUNC_ADD, GL_ONE, GL_ONE, so we just need a bool:
	bool blend;						//default false
	bool is_shadow_pass;			//default false

	Pass(Framebuffer* fb);		//Sets framebuffer and the defaults shown above.
	void start() const;

	static const Pass* current;
};


void init_screenspace();
void resize_screenbuffers(int w, int h);
inline bool s_is_shadow_pass() {return Pass::current->is_shadow_pass;}


void draw_fsq();

//half-screen quad, left and right
void draw_hsq(int i);

//quarter-screen quad, ul, ur, ll, lr
void draw_qsq(int i);
