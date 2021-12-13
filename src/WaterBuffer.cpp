#include "WaterBuffer.h"

#include <iostream>

#define REFLECTION_WIDTH 1270
#define REFLECTION_HEIGHT 720
#define REFRACTION_WIDTH 1270
#define REFRACTION_HEIGHT 720

WaterBuffer::WaterBuffer(int width, int height) {
	initializeReflectionFrameBuff(width, height);
	initializeRefractionFrameBuff(width, height);
}
WaterBuffer::~WaterBuffer() {
	glDeleteFramebuffers(1, &reflectionFrameBuff);
	glDeleteTextures(1, &reflectionTexture);
	glDeleteRenderbuffers(1, &reflectionDepthBuff);

	glDeleteFramebuffers(1, &refractionFrameBuff);
	glDeleteTextures(1, &refractionTexture);
	glDeleteRenderbuffers(1, &refractionDepthTexture);
}

void WaterBuffer::initializeReflectionFrameBuff(int width, int height) {
	reflectionFrameBuff = createFrameBuff();
	reflectionTexture = createTextureAttachment(REFLECTION_WIDTH, REFLECTION_HEIGHT);
	reflectionDepthBuff = createDepthBuffAttachment(REFLECTION_WIDTH, REFLECTION_HEIGHT);
	unbindCurrentFrameBuff(width, height);
}
void WaterBuffer::initializeRefractionFrameBuff(int width, int height) {
	refractionFrameBuff = createFrameBuff();
	refractionTexture = createTextureAttachment(REFRACTION_WIDTH, REFRACTION_HEIGHT);
	refractionDepthTexture = createDepthTextureAttachment(REFRACTION_WIDTH, REFRACTION_HEIGHT);
	unbindCurrentFrameBuff(width, height);
}

void WaterBuffer::bindReflectionFrameBuff() {
	bindFrameBuff(reflectionFrameBuff, REFLECTION_WIDTH, REFLECTION_HEIGHT);
}
void WaterBuffer::bindRefractionFrameBuff() {
	bindFrameBuff(refractionFrameBuff, REFRACTION_WIDTH, REFRACTION_HEIGHT);
}

void WaterBuffer::bindFrameBuff(GLuint frameBuff, int width, int height) {
	glBindTexture(GL_TEXTURE_2D, 0);	//ensure  texture is not bound
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuff);
	glViewport(0, 0, width, height);
}
void WaterBuffer::unbindCurrentFrameBuff(int width, int height) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
}

GLuint WaterBuffer::createFrameBuff() {
	//generate name for frame buffer
	GLuint frameBuff;
	glGenFramebuffers(1, &frameBuff);
	//create the framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuff);
	// always render to color attachment 0
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	return frameBuff;
}
GLuint WaterBuffer::createTextureAttachment(int width, int height) {
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
	return texture;
}
GLuint WaterBuffer::createDepthTextureAttachment(int width, int height) {
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture, 0);
	return texture;
}
GLuint WaterBuffer::createDepthBuffAttachment(int width, int height) {
	GLuint depthBuff;
	glGenRenderbuffers(1, &depthBuff);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuff);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuff);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	return depthBuff;
}