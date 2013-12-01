#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <GLES2/gl2.h>
#include <SOIL/SOIL.h>
#include "u2gl.h"

static int view_width;
static int view_height;

static struct u2gl_program triangle_program;
static struct u2gl_program fb_program;

static GLuint framebuffer = 0;

float obj[9];

float fb_obj[12] = {
	0.0f, 0.0f, 0.0f,
	320.0f, 0.0f, 0.0f,
	0.0f, 200.0f, 0.0f,
	320.0f, 200.0f, 0.0f
};

static const char vertex_shader[] =
"uniform mat4 pMatrix;\n"
"uniform mat4 uMatrix;\n"
"attribute vec4 aPosition;\n"
"varying vec3 vPosition;\n"
"\n"
"void main(){\n"
"    mat4 Matrix = pMatrix * uMatrix;\n"
"    vec4 position = Matrix * aPosition;\n"
"    gl_Position = position;\n"
"    vPosition = vec3(position);\n"
"}\n";

static const char fragment_shader[] =
"precision mediump float;\n"
"uniform vec4 uColor;\n"
"varying vec3 vPosition;\n"
"\n"
"void main() {\n"
"    gl_FragColor = uColor;\n"
"}\n";

static const char vertex_shader_texture[] =
"uniform mat4 pMatrix;\n"
"uniform mat4 uMatrix;\n"
"attribute vec4 aPosition;\n"
"varying vec3 vPosition;\n"
"attribute vec2 aTexPosition;\n"
"varying vec2 vTexPosition;\n"
"\n"
"void main() {\n"
"    mat4 Matrix = pMatrix * uMatrix;\n"
"    vec4 position = Matrix * aPosition;\n"
"    gl_Position = position;\n"
"    vTexPosition = aTexPosition;\n"
"    vPosition = vec3(position);\n"
"}\n";

static const char fragment_shader_texture[] =
"precision mediump float;\n"
"uniform sampler2D uTexture;\n"
"uniform vec4 uColor;\n"
"varying vec3 vPosition;\n"
"varying vec2 vTexPosition;\n"
"\n"
"void main() {\n"
"    gl_FragColor = texture2D(uTexture, vTexPosition).rgba;\n"
"}\n";

static const char fragment_shader_fbtexture[] =
"precision mediump float;\n"
"uniform sampler2D uTexture;\n"
"uniform vec4 uColor;\n"
"varying vec3 vPosition;\n"
"varying vec2 vTexPosition;\n"
"\n"
"void main() {\n"
"    const vec4 color0 = vec4(0.332,0.301,0.395,1.0);\n"
"    const vec4 color1 = vec4(0.457,0.395,0.523,1.0);\n"
"    const vec4 color2 = vec4(0.602,0.555,0.664,1.0);\n"
//"    const vec4 color2 = vec4(1.0,0.0,0.0,1.0);\n"
"    const vec4 color3 = vec4(0.742,0.695,0.809,1.0);\n"
//"    const vec4 color3 = vec4(0.0,1.0,0.0,1.0);\n"
"    float c = texture2D(uTexture, vTexPosition).r;\n"
"    if (c > 0.1) { if (c < 0.3) gl_FragColor = color0;\n"
"    else if (c < 0.45) gl_FragColor = color1;\n"
"    else if (c < 0.52) gl_FragColor = color2;\n"
"    else gl_FragColor = color3; }\n"
"    else gl_FragColor = texture2D(uTexture, vTexPosition).rgba;\n"
"}\n";



static float tex_coords[] = {
	0.0f, 1.0f,
	1.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 0.0f
};

static int fb_location;


static float color[256][4];

#define CC 256

void setrgb(int c, int r, int g, int b)
{
	color[c][0] = (float)r / CC;
	color[c][1] = (float)g / CC;
	color[c][2] = (float)b / CC;
	color[c][3] = 0.5f;
}

void getrgb(int c, char *p)
{
	p[0] = color[c][0] * CC;
	p[1] = color[c][1] * CC;
	p[2] = color[c][2] * CC;
}

void draw_fb()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(fb_program.program);
	u2gl_set_color(color[15], &fb_program);
	u2gl_draw_textured_triangle_strip(&fb_program, fb_obj, 4);
}

#if 0
static void draw_triangle(float *f, int c)
{
	u2gl_set_color(color[c], &triangle_program);

	obj[0] = *f++;
	obj[1] = *f++;

	obj[3] = *f++;
	obj[4] = *f++;

	obj[6] = *f++;
	obj[7] = *f++;

	u2gl_draw_triangle_strip(&triangle_program, obj, 3);
}
#endif

void draw_quad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glUseProgram(triangle_program.program);
	u2gl_set_color(color[1], &triangle_program);

	obj[0] = x1;
	obj[1] = y1;

	obj[3] = x2;
	obj[4] = y2;

	obj[6] = x4;
	obj[7] = y4;

	obj[9] = x3;
	obj[10] = y3;

	u2gl_draw_triangle_strip(&triangle_program, obj, 4);
}

void draw_palette()
{
	int i;

	glUseProgram(triangle_program.program);
	for (i = 0; i < 256; i++) {
		u2gl_set_color(color[i], &triangle_program);
	
		obj[0] = 20 + (i - 64 * (i / 64)) * 4;
		obj[1] = 45 -10 * (i / 64) ;
		obj[3] = obj[0];
		obj[4] = obj[1] - 10;
		obj[6] = obj[0] + 4;
		obj[7] = obj[1] - 10;
	
		u2gl_draw_triangle_strip(&triangle_program, obj, 3);
	}
}

static void init_texture()
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	u2gl_set_tex_coords(tex_coords);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height;
	unsigned char* image = SOIL_load_image("sr-port.png",
				&width, &height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
				GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);

	glGenerateMipmap(GL_TEXTURE_2D);
	u2gl_check_error("init_texture");
}

extern int window_width;
extern int window_height;

int init_opengl()
{
	Matrix m;
	GLuint v, f;
	GLuint fb_texture;

	view_width = 320;
	view_height = 200;

	glActiveTexture(GL_TEXTURE0);

// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
glGenFramebuffers(1, &framebuffer);
glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
// The texture we're going to render to
glGenTextures(1, &fb_texture);
// "Bind" the newly created texture : all future texture functions will modify this texture
glBindTexture(GL_TEXTURE_2D, fb_texture);
u2gl_set_tex_coords(tex_coords);
// Give an empty image to OpenGL ( the last "0" )
glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, window_width, window_height, 0,GL_RGB, GL_UNSIGNED_BYTE, 0);
// Poor filtering. Needed !
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
// Set "fb_texture" as our colour attachement #0
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb_texture, 0);
// Set the list of draw buffers.
//GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
//glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
	u2gl_check_error("framebuffer texture");


	v = u2gl_compile_vertex_shader(vertex_shader);
	f = u2gl_compile_fragment_shader(fragment_shader);
	u2gl_create_program(&triangle_program, f, v);

	v = u2gl_compile_vertex_shader(vertex_shader_texture);
	f = u2gl_compile_fragment_shader(fragment_shader_fbtexture);
	u2gl_create_program(&fb_program, f, v);

	fb_location = glGetUniformLocation(fb_program.program, "uTexture");


	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(.0, .0, .0, 0);

	glUseProgram(triangle_program.program);
	matrix_identity(m);
	u2gl_set_matrix(&triangle_program, m);

	glUseProgram(fb_program.program);
	u2gl_set_matrix(&fb_program, m);

	u2gl_check_error("init_opengl");

	u2gl_projection(0, view_width, 0, view_height, &triangle_program);
	u2gl_projection(0, view_width, 0, view_height, &fb_program);

	return 0;
}

void blend_alpha()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

void blend_color()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void clear_screen()
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glClear(GL_COLOR_BUFFER_BIT);
}