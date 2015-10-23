// Project for CS 637
// Shangqi Wu

#include "Angel.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

typedef vec4 color4;

// Height and width of main window. 
const int h = 1000;
const int w = 1000;

// True for perspective projection, false for parallel projection. 
bool perspective = true;

// RGBA color for background of main window.
float Red = 0;
float Green = 0;
float Blue = 0;
float Alpha = 1;

// Radius of camera rotation and its delta.
float cameraRadius = 0.7;
float dr = 0.05;
// Height of camera and its delta.
float cameraHeight = 0.7;
float dh = 0.05;
// Current position and its delta of a circling camera
float t = 0;
float dt = 0.01;

// Initial position of look-at, camera position, and up vector for projection.
const vec4 at(0, 0, 0, 1);
vec4 eye(0, 0, 0, 1);
const vec4 up(0, 1, 0, 1);

// Phong shading model parameters of light source 1, which rotates around the object.
float Idr1 = 0.6*0.3;
float Idg1 = 0.6*0.5;
float Idb1 = 0.6*0.5;
float Isr1 = 0.2;
float Isg1 = 0.2;
float Isb1 = 0.2;
float Iar1 = 0.05;
float Iag1 = 0.05;
float Iab1 = 0.05;

// Phong shading model parameters of light source 2, which moves with camera.
float Idr2 = 0.3*116.0/255.0;
float Idg2 = 0.3*222.0/255.0;
float Idb2 = 0.3*169.0/255.0;
float Isr2 = 0.2;
float Isg2 = 0.2;
float Isb2 = 0.2;
float Iar2 = 0.05*116.0 / 255.0;
float Iag2 = 0.05*222.0 / 255.0;
float Iab2 = 0.05*169.0 / 255.0;

// For the still light source color.
float Idr3 = 0.5*234.0 / 255.0;
float Idg3 = 0.5*136.0 / 255.0;
float Idb3 = 0.5*222.0 / 255.0;
float Isr3 = 0.2;
float Isg3 = 0.2;
float Isb3 = 0.2;
float Iar3 = 0.05*234.0 / 255.0;
float Iag3 = 0.05*136.0 / 255.0;
float Iab3 = 0.05*222.0 / 255.0;

// Shininess parameter for phong shading.
float shininess = 100;

// Phong shading model parameters of material property. 
float kdr = 1;
float kdg = 1;
float kdb = 1;
float ksr = 1;
float ksg = 1;
float ksb = 1;
float kar = 1;
float kag = 1;
float kab = 1;

// Position parameters of light source 2. 
float lightHeight = 1;
float lightRadius = 1;
float dhlight2 = 0.2;
float drlight2 = 0.2;
float tlight = 0;
float dtlight = 0.03;

// IDs for main window. 
int MainWindow;

// Vector containing vertices of every triangle to be drawn.
vector<vec4> vertices;
// Vector for average normal for corresponding vertice in a triangle. 
vector<vec4> normals;

// ID for shaders programs.
GLuint program, shadowShader;
// ID for FBOs and their components. 
GLuint renderFBO, renderColor, renderDepth, checkerBoard;
GLuint depthFBO_light1, depthFBO_light2, depthFBO_light3;
// IDs for shadow textures. 
GLuint depthTexture_light1, depthTexture_light2, depthTexture_light3;

//--------------------------------------------------------------------------

vec4
product(const vec4 &a, const vec4 &b)
{
	return vec4(a[0] * b[0], a[1] * b[1], a[2] * b[2], a[3] * b[3]);
}

//--------------------------------------------------------------------------

void
init(void)
{
	// Create a FBO for normal scene rendering.
	glGenFramebuffersEXT(1, &renderFBO);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, renderFBO);

	GLint samples;
	glGetIntegerv(GL_MAX_SAMPLES, &samples);

	// Render buffer for color.
	glGenRenderbuffers(1, &renderColor);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderColor);
	glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, samples, GL_RGBA8, w, h);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, renderColor);

	// Render buffer for depth.
	glGenRenderbuffersEXT(1, &renderDepth);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderDepth);
	glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, samples, GL_DEPTH_COMPONENT24, w, h);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, renderDepth);

	// Check status for scene buffer.
	GLenum FBOstatus = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (FBOstatus != GL_FRAMEBUFFER_COMPLETE) {
		cerr << "Status of render FBO is incomplete." << endl;
		exit(1);
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	// FBO for shadow calculation.
	glGenFramebuffersEXT(1, &depthFBO_light1);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, depthFBO_light1);
	// The depth buffer texture  
	glGenTextures(1, &depthTexture_light1);
	glBindTexture(GL_TEXTURE_2D, depthTexture_light1);

	// Texture parameters.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 2 * w, 2 * h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

	// Bind texture to shadow mapping FBO.
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, depthTexture_light1, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glClearDepth(1.0); 

	// Check status.
	FBOstatus = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (FBOstatus != GL_FRAMEBUFFER_COMPLETE) {
		cerr << "Status of depth texture is incomplete." << endl;
		exit(1);
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	// FBO for shadow calculation.
	glGenFramebuffersEXT(1, &depthFBO_light2);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, depthFBO_light2);
	// The depth buffer texture  
	glGenTextures(1, &depthTexture_light2);
	glBindTexture(GL_TEXTURE_2D, depthTexture_light2);

	// Texture parameters.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 2 * w, 2 * h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

	// Bind texture to shadow mapping FBO.
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, depthTexture_light2, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glClearDepth(1.0); 

	// Check status.
	FBOstatus = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (FBOstatus != GL_FRAMEBUFFER_COMPLETE) {
		cerr << "Status of depth texture is incomplete." << endl;
		exit(1);
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	// FBO for shadow calculation.
	glGenFramebuffersEXT(1, &depthFBO_light3);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, depthFBO_light3);
	// The depth buffer texture  
	glGenTextures(1, &depthTexture_light3);
	glBindTexture(GL_TEXTURE_2D, depthTexture_light3);

	// Texture parameters.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 2 * w, 2 * h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

	// Bind texture to shadow mapping FBO.
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, depthTexture_light3, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glClearDepth(1.0); 

	// Check status.
	FBOstatus = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (FBOstatus != GL_FRAMEBUFFER_COMPLETE) {
		cerr << "Status of depth texture is incomplete." << endl;
		exit(1);
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	// Generate 2D texture for the mat. 
	glGenTextures(1, &checkerBoard);
	glBindTexture(GL_TEXTURE_2D, checkerBoard);
	GLubyte* checkerData = new GLubyte[4 * h * w];
	for (int i = 0; i < h; i++)
		for (int j = 0; j < w; j++) {
			GLubyte c = (((i & 0x16) == 0) ^ ((j & 0x16) == 0)) * 255;
			checkerData[4 * i * w + 4 * j] = c;
			checkerData[4 * i * w + 4 * j + 1] = c;
			checkerData[4 * i * w + 4 * j + 2] = c;
			checkerData[4 * i * w + 4 * j + 3] = 255;
		}

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, checkerData);
	glGenerateMipmap(GL_TEXTURE_2D);

	delete checkerData;
	checkerData = NULL;
	
	// Create a vertex array object.
	GLuint vao[1];
	glGenVertexArrays(1, vao);
	glBindVertexArray(vao[0]);

	vec4 checkerBoardCoord[6] = {
		vec4(1, 0, 0, 1), vec4(1, 1, 0, 1), vec4(0, 0, 0, 1),
		vec4(0, 0, 0, 1), vec4(1, 1, 0, 1), vec4(0, 1, 0, 1)
	};

	// Create and initialize a buffer object.
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(vec4) + normals.size()*sizeof(vec4) + sizeof(checkerBoardCoord), NULL, GL_STATIC_DRAW);
	// Pass vertices & normals data to opengl buffer object.
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size()*sizeof(vec4), vertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertices.size()*sizeof(vec4), normals.size()*sizeof(vec4), normals.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertices.size()*sizeof(vec4) + normals.size()*sizeof(vec4), sizeof(checkerBoardCoord), checkerBoardCoord);

	shadowShader = InitShader("vshadershadow.glsl", "fshadershadow.glsl");
	glBindAttribLocation(shadowShader, 0, "vVertex");
	glBindAttribLocation(shadowShader, 0, "fragmentdepth");
	LinkShader(shadowShader);

	GLuint loc_ver_shadow = glGetAttribLocation(shadowShader, "vVertex");
	glEnableVertexAttribArray(loc_ver_shadow);
	glVertexAttribPointer(loc_ver_shadow, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	// Load shaders and use the resulting shader program.
	program = InitShader("vshader21.glsl", "fshader21.glsl");
	LinkShader(program);

	// Initialize the vertex position attribute from the vertex shader.
	GLuint loc_ver = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(loc_ver);
	glVertexAttribPointer(loc_ver, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	// Pass normal vectors of each triangle to vertex shader
	GLuint loc_col = glGetAttribLocation(program, "vNormal");
	glEnableVertexAttribArray(loc_col);
	glVertexAttribPointer(loc_col, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(vertices.size()*sizeof(vec4)));

	// Pass normal vectors of each triangle to vertex shader
	GLuint loc_che = glGetAttribLocation(program, "checkerBoardCoord");
	glEnableVertexAttribArray(loc_che);
	glVertexAttribPointer(loc_che, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(vertices.size()*sizeof(vec4) + normals.size()*sizeof(vec4)));
}

//----------------------------------------------------------------------------

void
display(void)
{
	// Calculate renewed camera position. 
	eye = vec4(cameraRadius*sin(t), cameraHeight, cameraRadius*cos(t), 1);

	// Light 1 is within camera coordinate.
	vec4 light1_pos = vec4((cameraRadius+1)*sin(t), cameraHeight, (cameraRadius+1)*cos(t), 1);
	color4 light1_diffuse = color4(Idr1, Idg1, Idb1, 1);
	color4 light1_specular = color4(Isr1, Isg1, Isb1, 1);
	color4 light1_ambient = color4(Iar1, Iag1, Iab1, 1);

	// Light 2 is within world coordinate.
	vec4 light2_pos = vec4(lightRadius*sin(tlight), lightHeight, lightRadius*cos(tlight), 1);
	color4 light2_diffuse = color4(Idr2, Idg2, Idb2, 1);
	color4 light2_specular = color4(Isr2, Isg2, Isb2, 1);
	color4 light2_ambient = color4(Iar2, Iag2, Iab2, 1);

	// Another still light source 3, magenta light
	vec4 light3_pos = vec4(0.7, 1, -0.7, 1);
	color4 light3_diffuse = color4(Idr3, Idg3, Idb3, 1);
	color4 light3_specular = color4(Isr3, Isg3, Isb3, 1);
	color4 light3_ambient = color4(Iar3, Iag3, Iab3, 1);

	// Material property.
	color4 material_diffuse(kdr, kdg, kdb, 1);
	color4 material_ambient(kar, kag, kab, 1);
	color4 material_specular(ksr, ksg, ksb, 1);

	// Create model and projection matrix.
	mat4 modelview;
	mat4 projection;

	// Implementing projection.
	if (perspective) projection = Perspective(90, 1, 1e-10, 1e10);
	else projection = Ortho(-2, 2, -2, 2, -100, 100);

	// Implementing modelview. 
	modelview = LookAt(eye, at, up) * modelview;

	glUseProgram(program);
	// Pass model and projection matrix to vertex shader. 
	GLuint loc_modelview = glGetUniformLocation(program, "modelview");
	glUniformMatrix4fv(loc_modelview, 1, GL_TRUE, modelview);
	GLint loc_projection = glGetUniformLocation(program, "projection");
	glUniformMatrix4fv(loc_projection, 1, GL_TRUE, projection);
	GLint loc_eyeposition = glGetUniformLocation(program, "eyeposition");
	glUniform4f(loc_eyeposition, eye.x, eye.y, eye.z, eye.w);

	// Pass positions of light sources to vertex shader.
	GLint loc_light1_pos = glGetUniformLocation(program, "light1_pos");
	GLint loc_light2_pos = glGetUniformLocation(program, "light2_pos");
	GLint loc_light3_pos = glGetUniformLocation(program, "light3_pos");
	glUniform4f(loc_light1_pos, light1_pos.x, light1_pos.y, light1_pos.z, light1_pos.w);
	glUniform4f(loc_light2_pos, light2_pos.x, light2_pos.y, light2_pos.z, light2_pos.w);
	glUniform4f(loc_light3_pos, light3_pos.x, light3_pos.y, light3_pos.z, light3_pos.w);

	// Calculate and pass color products of each light source to vertex shader.
	vec4 d_pro1 = product(light1_diffuse, material_diffuse);
	vec4 d_pro2 = product(light2_diffuse, material_diffuse);
	vec4 d_pro3 = product(light3_diffuse, material_diffuse);
	vec4 a_pro1 = product(light1_ambient, material_ambient);
	vec4 a_pro2 = product(light2_ambient, material_ambient);
	vec4 a_pro3 = product(light3_ambient, material_ambient);
	vec4 s_pro1 = product(light1_specular, material_specular);
	vec4 s_pro2 = product(light2_specular, material_specular);
	vec4 s_pro3 = product(light3_specular, material_specular);
	GLint loc_diffuse_product1 = glGetUniformLocation(program, "light1_diffuse_product");
	GLint loc_diffuse_product2 = glGetUniformLocation(program, "light2_diffuse_product");
	GLint loc_diffuse_product3 = glGetUniformLocation(program, "light3_diffuse_product");
	GLint loc_specular_product1 = glGetUniformLocation(program, "light1_specular_product");
	GLint loc_specular_product2 = glGetUniformLocation(program, "light2_specular_product");
	GLint loc_specular_product3 = glGetUniformLocation(program, "light3_specular_product");
	GLint loc_ambient_product1 = glGetUniformLocation(program, "light1_ambient_product");
	GLint loc_ambient_product2 = glGetUniformLocation(program, "light2_ambient_product");
	GLint loc_ambient_product3 = glGetUniformLocation(program, "light3_ambient_product");
	glUniform4f(loc_diffuse_product1, d_pro1.x, d_pro1.y, d_pro1.z, d_pro1.w);
	glUniform4f(loc_diffuse_product2, d_pro2.x, d_pro2.y, d_pro2.z, d_pro2.w);
	glUniform4f(loc_diffuse_product3, d_pro3.x, d_pro3.y, d_pro3.z, d_pro3.w);
	glUniform4f(loc_specular_product1, s_pro1.x, s_pro1.y, s_pro1.z, s_pro1.w);
	glUniform4f(loc_specular_product2, s_pro2.x, s_pro2.y, s_pro2.z, s_pro2.w);
	glUniform4f(loc_specular_product3, s_pro3.x, s_pro3.y, s_pro3.z, s_pro3.w);
	glUniform4f(loc_ambient_product1, a_pro1.x, a_pro1.y, a_pro1.z, a_pro1.w);
	glUniform4f(loc_ambient_product2, a_pro2.x, a_pro2.y, a_pro2.z, a_pro2.w);
	glUniform4f(loc_ambient_product3, a_pro3.x, a_pro3.y, a_pro3.z, a_pro3.w);
	GLint loc_shininess = glGetUniformLocation(program, "shininess");
	glUniform1f(loc_shininess, shininess);

	glEnable(GL_CULL_FACE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	// Create matrix from still light view. 
	mat4 ShadowModel_light1;
	mat4 ShadowModel_light2;
	mat4 ShadowModel_light3;
	mat4 ShadowProj;
	mat4 DepthBiasMatrix = Translate(0.5, 0.5, 0.5) * Scale(0.5, 0.5, 0.5);
	ShadowModel_light1 = LookAt(light1_pos, at, up) * ShadowModel_light1;
	ShadowModel_light2 = LookAt(light2_pos, at, up) * ShadowModel_light2;
	ShadowModel_light3 = LookAt(light3_pos, at, up) * ShadowModel_light3;
	ShadowProj = Perspective(90, 1, 1e-10, 1e10);

	// Render shadow mapping depth texture.
	glUseProgram(shadowShader);
	glEnable(GL_TEXTURE_2D);
	/*
	 * Start to render depth texture for light source 1. 
	 */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, depthFBO_light1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(Red, Green, Blue, Alpha);
	glViewport(0, 0, 2*w, 2*h);

	// Pass shadow matrix to shadow mapping shader.
	GLuint loc_shadowmodel = glGetUniformLocation(shadowShader, "shadowmodel");
	glUniformMatrix4fv(loc_shadowmodel, 1, GL_TRUE, ShadowModel_light1);
	GLuint loc_shadowproj = glGetUniformLocation(shadowShader, "shadowproj");
	glUniformMatrix4fv(loc_shadowproj, 1, GL_TRUE, ShadowProj);

	glCullFace(GL_FRONT);
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	/*
	 * Start to render depth texture for light source 2.
	 */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, depthFBO_light2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(Red, Green, Blue, Alpha);
	glViewport(0, 0, 2 * w, 2 * h);

	// Pass shadow matrix to shadow mapping shader.
	loc_shadowmodel = glGetUniformLocation(shadowShader, "shadowmodel");
	glUniformMatrix4fv(loc_shadowmodel, 1, GL_TRUE, ShadowModel_light2);

	glCullFace(GL_FRONT);
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	/*
	 * Start to render depth texture for light source 3.
	 */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, depthFBO_light3);
	glBindTexture(GL_TEXTURE_2D, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(Red, Green, Blue, Alpha);
	glViewport(0, 0, 2*w, 2*h);

	// Pass shadow matrix to shadow mapping shader.
	loc_shadowmodel = glGetUniformLocation(shadowShader, "shadowmodel");
	glUniformMatrix4fv(loc_shadowmodel, 1, GL_TRUE, ShadowModel_light3);

	glCullFace(GL_FRONT);
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	/*
	 * Start to render real scene.
	 */
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	// Switching shader program.
	glUseProgram(program);
	// Switching FBO for rendering.
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, renderFBO);
	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, renderFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(Red, Green, Blue, Alpha); // Set background color of main window.
	glViewport(0, 0, w, h);
	glCullFace(GL_BACK);
        glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthTexture_light1);
        glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthTexture_light2);
        glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, depthTexture_light3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, checkerBoard);

	// Pass shadow texture to shader.
	loc_shadowmodel = glGetUniformLocation(program, "shadowmodel_light1");
	glUniformMatrix4fv(loc_shadowmodel, 1, GL_TRUE, ShadowModel_light1);
	loc_shadowmodel = glGetUniformLocation(program, "shadowmodel_light2");
	glUniformMatrix4fv(loc_shadowmodel, 1, GL_TRUE, ShadowModel_light2);
	loc_shadowmodel = glGetUniformLocation(program, "shadowmodel_light3");
	glUniformMatrix4fv(loc_shadowmodel, 1, GL_TRUE, ShadowModel_light3);
	loc_shadowproj = glGetUniformLocation(program, "shadowproj");
	glUniformMatrix4fv(loc_shadowproj, 1, GL_TRUE, ShadowProj);
	GLuint loc_depthbias = glGetUniformLocation(program, "depthbiasmatrix");
	glUniformMatrix4fv(loc_depthbias, 1, GL_TRUE, DepthBiasMatrix);
	GLuint loc_shadow = glGetUniformLocation(program, "shadowMap_light1");
	glUniform1i(loc_shadow, 0);
	loc_shadow = glGetUniformLocation(program, "shadowMap_light2");
	glUniform1i(loc_shadow, 1);
	loc_shadow = glGetUniformLocation(program, "shadowMap_light3");
	glUniform1i(loc_shadow, 2);
	GLuint loc_cb = glGetUniformLocation(program, "checkerBoard");
	glUniform1i(loc_cb, 3);

	GLuint loc_obj = glGetUniformLocation(program, "obj");
	glUniform1i(loc_obj, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glUniform1i(loc_obj, 1);
	glDrawArrays(GL_TRIANGLES, 6, vertices.size() - 6); // Draw the points by one triangle.

	// Finish rendering and copy image to screen.
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
	glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, renderFBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBlitFramebufferEXT(0, 0, w - 1, h - 1, 0, 0, w - 1, h - 1, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glutSwapBuffers(); // Double buffer swapping. 
	glFlush(); // Flush. 
	glDisable(GL_CULL_FACE);
}

//----------------------------------------------------------------------------

void
RotationFunc(void)
{
	t += dt; // Camera rotation animation.
	tlight += dtlight;
	glutPostRedisplay(); // Redisplay function.
}

//----------------------------------------------------------------------------

void
keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case 033: exit(EXIT_SUCCESS); break; // "Esc": exit the program.
	case (int)'w': cameraHeight += dh; break; // Increasing camera height.
	case (int)'s': cameraHeight -= dh; break; // Decreasing camera height.
	case (int)'a': cameraRadius += dr; break;// Incresing camera radius, the object looks smaller under perspective projection.
	case (int)'d': if (cameraRadius > dr) cameraRadius -= dr; break; // Decreasing camera radius, the object looks larger under perspective projection.
	case (int)'e': dt += 0.0005; break; // Double camera rotation speed.
	case (int)'q': dt -= 0.0005; break; // Half camera rotation speed.
	case (int)'z': t -= dt; break;
	case (int)'x': t += dt; break;
	case (int)'t': lightHeight += dhlight2; break; // Increasing light height.
	case (int)'g': lightHeight -= dhlight2; break; // Decreasing light height.
	case (int)'h': lightRadius += drlight2; break; // Increasing light orbit radius, the light source becomes farther to the object.
	case (int)'f': if (lightRadius > drlight2) lightRadius -= drlight2; break; // Decreasing light orbit radius, the loght source becomes closer to the object. 
	case (int)'y': tlight += dtlight; break; // Rotate light 2 counter-clockwise.
	case (int)'r': tlight -= dtlight; break; // Rotate light 2 clockwise.
	case (int)'v': dtlight *= 2; break; // Make light 2 rotate 2x faster.
	case (int)'c': dtlight /= 2; break; // Make light 2 rotate half speed.
	}
	glutPostRedisplay();
}

//----------------------------------------------------------------------------

void
MainSubMenuRotation(int id)
{
	switch (id) {
	case 1: glutIdleFunc(RotationFunc); break; // Start or stop camera rotation.
	case 2: glutIdleFunc(NULL); break; // Start or stop light rotation.
	}
	glutPostRedisplay();
}

//----------------------------------------------------------------------------
void
MainSubMenuMaterial(int id)
{
	switch (id) {
	case 1: // White plastic. (Default)
		kdr = 1; kdg = 1; kdb = 1;
		ksr = 1; ksg = 1; ksb = 1;
		kar = 1; kag = 1; kab = 1;
		shininess = 100;
		break;
	case 2: // Gold.
		kdr = 1; kdg = 215.0 / 255.0; kdb = 0;
		ksr = 5; ksg = 5; ksb = 5;
		kar = 0.5; kag = 0.5*215.0 / 255.0; kab = 0.5;
		shininess = 5000;
		break;
	case 3: // Silver.
		kdr = 233.0 / 255.0; kdg = 233.0 / 255.0; kdb = 216.0 / 255.0;
		ksr = 0.1; ksg = 0.1; ksb = 0.1;
		kar = 233.0 / 255.0; kag = 233.0 / 255.0; kab = 233.0 / 255.0;
		shininess = 10;
		break;
	}
	glutPostRedisplay();
}

//----------------------------------------------------------------------------

void
MainSubMenuPerspective(int id)
{
	switch (id) {
	case 1: perspective = true; break; // Switch to persepctive projection.
	case 2: perspective = false; break; // Switch to parallel projection.
	}
	glutPostRedisplay();
}

//----------------------------------------------------------------------------
void
MainSubMenuLight(int id)
{
	switch (id) {
	case 1: // Light 1: cyan-like, moving with camera; light 2: turqoise, rotating around object, light 3: magenta, still light. 
		Idr1 = 0.6*0.3; Idg1 = 0.6*0.5; Idb1 = 0.6*0.5;
		Isr1 = 0.2; Isg1 = 0.2; Isb1 = 0.2; 
		Iar1 = 0.05; Iag1 = 0.05; Iab1 = 0.05;
		Idr2 = 0.3*116.0 / 255.0; Idg2 = 0.3*222.0 / 255.0; Idb2 = 0.3*169.0 / 255.0;
		Isr2 = 0.2; Isg2 = 0.2; Isb2 = 0.2;
		Iar2 = 0.05*116.0 / 255.0; Iag2 = 0.05*222.0 / 255.0; Iab2 = 0.05*169.0 / 255.0;
		Idr3 = 0.5*234.0 / 255.0; Idg3 = 0.5*136.0 / 255.0; Idb3 = 0.5*222.0 / 255.0; 
		Isr3 = 0.2; Isg3 = 0.2; Isb3 = 0.2;
		Iar3 = 0.05*234.0 / 255.0; Iag3 = 0.05*136.0 / 255.0; Iab3 = 0.05*222.0 / 255.0;
		break;
	case 2: // Both light change to white.
		Idr1 = 0.4; Idg1 = 0.4; Idb1 = 0.4;
		Isr1 = 0.2; Isg1 = 0.2; Isb1 = 0.2;
		Iar1 = 0.05; Iag1 = 0.05; Iab1 = 0.05;
		Idr2 = 0.3; Idg2 = 0.3; Idb2 = 0.3;
		Isr2 = 0.2; Isg2 = 0.2; Isb2 = 0.2;
		Iar2 = 0.05; Iag2 = 0.05; Iab2 = 0.05;
		Idr3 = 0.3; Idg3 = 0.3; Idb3 = 0.3;
		Isr3 = 0.2; Isg3 = 0.2; Isb3 = 0.2;
		Iar3 = 0.05; Iag3 = 0.05; Iab3 = 0.05;
		break;
	case 3: // Light 1 only.
		Idr1 = 1.3*0.6*0.3; Idg1 = 1.3*0.6*0.5; Idb1 = 1.3*0.6*0.5;
		Isr1 = 1.3*0.2; Isg1 = 1.3*0.2; Isb1 = 1.3*0.2;
		Iar1 = 1.3*0.05; Iag1 = 1.3*0.05; Iab1 = 1.3* 0.05;
		Idr2 = 0; Idg2 = 0; Idb2 = 0;
		Isr2 = 0; Isg2 = 0; Isb2 = 0;
		Iar2 = 0; Iag2 = 0; Iab2 = 0;
		Idr3 = 0; Idg3 = 0; Idb3 = 0;
		Isr3 = 0; Isg3 = 0; Isb3 = 0;
		Iar3 = 0; Iag3 = 0; Iab3 = 0;
		break;
	case 4: // Light 2 only.
		Idr1 = 0; Idg1 = 0; Idb1 = 0;
		Isr1 = 0; Isg1 = 0; Isb1 = 0;
		Iar1 = 0; Iag1 = 0; Iab1 = 0;
		Idr2 = 1.3*0.3*116.0 / 255.0; Idg2 = 1.3*0.3*222.0 / 255.0; Idb2 = 1.3*0.3*169.0 / 255.0;
		Isr2 = 1.3*0.2; Isg2 = 1.3*0.2; Isb2 = 1.3*0.2;
		Iar2 = 1.3*0.05*116.0 / 255.0; Iag2 = 1.3*0.05*222.0 / 255.0; Iab2 = 1.3*0.05*169.0 / 255.0;
		Idr3 = 0; Idg3 = 0; Idb3 = 0;
		Isr3 = 0; Isg3 = 0; Isb3 = 0;
		Iar3 = 0; Iag3 = 0; Iab3 = 0;
		break;
	case 5: // Light 3 only.
		Idr1 = 0; Idg1 = 0; Idb1 = 0;
		Isr1 = 0; Isg1 = 0; Isb1 = 0;
		Iar1 = 0; Iag1 = 0; Iab1 = 0;
		Idr2 = 0; Idg2 = 0; Idb2 = 0;
		Isr2 = 0; Isg2 = 0; Isb2 = 0;
		Iar2 = 0; Iag2 = 0; Iab2 = 0;
		Idr3 = 1.3*0.5*234.0 / 255.0; Idg3 = 1.3*0.5*136.0 / 255.0; Idb3 = 1.3*0.5*222.0 / 255.0;
		Isr3 = 1.3*0.2; Isg3 = 1.3*0.2; Isb3 = 1.3*0.2;
		Iar3 = 1.3*0.05*234.0 / 255.0; Iag3 = 1.3*0.05*136.0 / 255.0; Iab3 = 1.3*0.05*222.0 / 255.0;
		break;
	}
	glutPostRedisplay();
}

//----------------------------------------------------------------------------

void
setMainWinMenu(void)
{
	int submenu_id_r, submenu_id_p, submenu_id_m, submenu_id_l;
	// Create submenu for rotating animation.
	submenu_id_r = glutCreateMenu(MainSubMenuRotation);
	glutAddMenuEntry("Start Camera Rotation", 1);
	glutAddMenuEntry("Stop Camera Rotation", 2);

	// Create submenu for projection changing.
	submenu_id_p = glutCreateMenu(MainSubMenuPerspective);
	glutAddMenuEntry("Perspective Projection", 1);
	glutAddMenuEntry("Parallel Projection", 2);

	// Create submenu for material selection.
	submenu_id_m = glutCreateMenu(MainSubMenuMaterial);
	glutAddMenuEntry("White Plastic", 1);
	glutAddMenuEntry("Gold", 2);
	glutAddMenuEntry("Silver", 3);

	// Create submenu for light control.
	submenu_id_l = glutCreateMenu(MainSubMenuLight);
	glutAddMenuEntry("Default Color Light", 1);
	glutAddMenuEntry("White Light", 2);
	glutAddMenuEntry("Light 1 Only", 3);
	glutAddMenuEntry("Light 2 Only", 4);
	glutAddMenuEntry("Light 3 Only", 5);

	glutCreateMenu(NULL); // Set menu in main window. 
	glutAddSubMenu("Camera Rotation", submenu_id_r);
	glutAddSubMenu("Projection", submenu_id_p);
	glutAddSubMenu("Material", submenu_id_m);
	glutAddSubMenu("Light Color", submenu_id_l);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

//----------------------------------------------------------------------------

void
readfile(void)
{
	// Start to read file.
	ifstream infile;
	string filename[3] = { "bound-cow.smf", "bound-bunny_5k.smf", "bound-lo-sphere.smf" };
	float offset_x[3] = { -1.1, 0, 1.1 };
	float offset_y[3] = { 0, 0.2, 0.2 };
	int fileNum = 0;

	// Push vertices and normals for the mat fist. 
	vertices.push_back(vec4(5, -0.3, 5, 1));
	vertices.push_back(vec4(5, -0.3, -5, 1));
	vertices.push_back(vec4(-5, -0.3, 5, 1));
	vertices.push_back(vec4(-5, -0.3, 5, 1));
	vertices.push_back(vec4(5, -0.3, -5, 1));
	vertices.push_back(vec4(-5, -0.3, -5, 1));
	normals.push_back(vec4(0, 1, 0, 1));
	normals.push_back(vec4(0, 1, 0, 1));
	normals.push_back(vec4(0, 1, 0, 1));
	normals.push_back(vec4(0, 1, 0, 1));
	normals.push_back(vec4(0, 1, 0, 1));
	normals.push_back(vec4(0, 1, 0, 1));

	do {
		infile.open(filename[fileNum].c_str());
		if (!infile.is_open()) continue;

		bool storev = false;
		bool storef = false;
		string str;
		vector<vec4> original_vertices;
		vector<vec4> original_normals;
		vector<vector<int> > faces;
		vector<double> ver_pos;
		vector<int> ver_no;

		// Read file content.
		while (infile) {
			infile >> str;
			if (str.compare("v") == 0) {
				storev = true;
			}
			else if (str.compare("f") == 0){
				storef = true;
			}
			else if (storev == true){ // Add vertex to its vector.
				// Store a vertice.
				ver_pos.push_back(atof(str.c_str()));
				if (ver_pos.size() == 3) {
					vec4 ver(ver_pos[0]+offset_x[fileNum], ver_pos[1]+offset_y[fileNum], ver_pos[2], 1);
					original_vertices.push_back(ver);
					storev = false;
					ver_pos.clear();
				}
			}
			else if (storef == true){ // Add face to its vector.
				ver_no.push_back(atoi(str.c_str()));
				// Store vertices for a triangle and calculate its normal vector.
				if (ver_no.size() == 3) {
					faces.push_back(ver_no);
					storef = false;
					ver_no.clear();
				}
			}
		}
		infile.close();

		original_normals.resize(original_vertices.size());
		for (int i = 0; i < (int) original_normals.size(); i++)
			original_normals[i] = vec4(0, 0, 0, 0);

		for (int i = 0; i < (int) faces.size(); i++) {
			vec4 tmpnorm = normalize(vec4(cross((original_vertices[faces[i][1] - 1] - original_vertices[faces[i][0] - 1]), (original_vertices[faces[i][2] - 1] - original_vertices[faces[i][0] - 1]))));
			original_normals[faces[i][0] - 1] = normalize(original_normals[faces[i][0] - 1] + tmpnorm);
			original_normals[faces[i][1] - 1] = normalize(original_normals[faces[i][1] - 1] + tmpnorm);
			original_normals[faces[i][2] - 1] = normalize(original_normals[faces[i][2] - 1] + tmpnorm);
		}

		for (int i = 0; i < (int) faces.size(); i++) {
			vertices.push_back(original_vertices[faces[i][0] - 1]);
			vertices.push_back(original_vertices[faces[i][1] - 1]);
			vertices.push_back(original_vertices[faces[i][2] - 1]);
			normals.push_back(original_normals[faces[i][0] - 1]);
			normals.push_back(original_normals[faces[i][1] - 1]);
			normals.push_back(original_normals[faces[i][2] - 1]);
		}
	} while (++fileNum < 3);
}

//----------------------------------------------------------------------------

int
main(int argc, char **argv)
{
	readfile(); // Read input smf file.

	glutInit(&argc, argv); // Initializing environment.
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE); // Enable depth.
	glutInitWindowSize(w, h);
	glutInitWindowPosition(0, 0);

	MainWindow = glutCreateWindow("ICG_Project"); // Initializing & setting main window.
	glewExperimental = GL_TRUE;
	glewInit();
	init(); // Initializing VAOs & VBOs. 
	glutDisplayFunc(display); // Setting display function for main window.
	glutKeyboardFunc(keyboard); // Setting keyboard function for main window.
	setMainWinMenu(); // Setting menu for main window. 
	glutIdleFunc(RotationFunc); // Start animation by default.

	glEnable(GL_MULTISAMPLE_ARB);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glutMainLoop(); // Start main loop. 
	return 0;
}
