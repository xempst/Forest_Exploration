#ifndef WATERBUFFER_H
#define WATERBUFFER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class WaterBuffer {
public:
	WaterBuffer(int width, int height);
	~WaterBuffer();

	void initializeReflectionFrameBuff(int width, int height);
	void initializeRefractionFrameBuff(int width, int height);

	void bindReflectionFrameBuff();
	void bindRefractionFrameBuff();

	void bindFrameBuff(GLuint frameBuff, int width, int height);
	void unbindCurrentFrameBuff(int width, int height);		// switch to default frame buffer

	GLuint createFrameBuff();
	GLuint createTextureAttachment(int width, int height);
	GLuint createDepthTextureAttachment(int width, int height);
	GLuint createDepthBuffAttachment(int width, int height);
	
	GLuint getReflectionTexture() { return reflectionTexture;  }
	GLuint getRefractionTexture() { return refractionTexture;  }
	GLuint getRefractionDepthTexture() { return refractionDepthTexture; }

private:
	GLuint reflectionFrameBuff;
	GLuint reflectionTexture;
	GLuint reflectionDepthBuff;

	GLuint refractionFrameBuff;
	GLuint refractionTexture;
	GLuint refractionDepthTexture;
};

#endif // !WATERBUFFER_H
