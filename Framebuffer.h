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

	GLuint make_texture(GLuint framebuffer, GLsizei width, GLsizei height);
	
	//There really isn't a good name for this, 'cuz all it does is call glTexImage*(), and that's not a good name.
	void tex_image(GLuint tex_name, GLsizei width, GLsizei height, void* data = NULL);
};

struct ExtraAttachment
{
	GLenum attachment_point;
	GLuint tex_name;
};


struct Framebuffer
{
	GLuint name;
	GLsizei width, height;

	const char* display_name;

	std::vector<GLuint> textures;
	std::vector<TextureSpec> texture_specs;

	Framebuffer(
		const char* display_name,
		std::vector<TextureSpec> texture_specs,
		std::vector<ExtraAttachment> extra_attachments,
		GLsizei width = 0,
		GLsizei height = 0
	);

	void check_status() const;
};

struct Screenbuffer : public Framebuffer
{
	virtual void post_resize();
	Screenbuffer(
		const char* display_name,
		std::vector<TextureSpec> texture_specs,
		std::vector<ExtraAttachment> extra_attachments,
		GLsizei width = 0,
		GLsizei height = 0
	);
	static std::vector<Screenbuffer*> all_screenbuffers;
	static void post_resize_all();
};

extern Screenbuffer* s_gbuffer;
#define s_gbuffer_albedo (s_gbuffer->textures[0])
#define s_gbuffer_position (s_gbuffer->textures[1])
#define s_gbuffer_normal (s_gbuffer->textures[2])
#define s_gbuffer_depth (s_gbuffer->textures[3])
#define s_gbuffer_index (s_gbuffer->textures[4])


struct Pass
{
	Framebuffer* framebuffer;		//NULL means default framebuffer.

	GLbitfield clear_mask;			//default color and depth
	bool depth_test, depth_mask;	//defaults true and true
	GLenum cull_face;				//0 means disable face culling. Default GL_BACK
	//So far, the only blending we need is GL_FUNC_ADD, GL_ONE, GL_ONE, so we just need a bool:
	bool blend;						//default false
	bool is_shadow_pass;			//default false

	Pass(Framebuffer* fb, Pass* copy = NULL);		//If copy is NULL, set the defaults above.
	void start() const;

	static const Pass* current;
};


void init_framebuffers();
void resize_screenbuffers(int w, int h);
inline bool s_is_shadow_pass() {return Pass::current->is_shadow_pass;}


void draw_fsq();

//half-screen quad, left and right
void draw_hsq(int i);

//quarter-screen quad, ul, ur, ll, lr
void draw_qsq(int i);
