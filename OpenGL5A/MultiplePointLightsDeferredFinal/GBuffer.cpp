
#include "GBuffer.h"

// fonctions d'aide

GLuint CreateRenderTexture(GLuint width, GLuint height, GLenum internalformat, GLenum format, GLenum type)
{
	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	// il est obligatoire de specifier une valeur pour GL_TEXTURE_MIN_FILTER
	// autrement le Texture Object est considere comme invalide
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);	

	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, nullptr);

	return texID;
}

GLuint CreateFBO()
{
	GLuint FBO;
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	return FBO;
}

// 

void GBuffer::Create()
{	
	FBO = CreateFBO();
	width = glutGet(GLUT_WINDOW_WIDTH);
	height = glutGet(GLUT_WINDOW_HEIGHT);	

	positionTex = CreateRenderTexture(width, height, GL_RGBA32F, GL_RGBA, GL_FLOAT);
	normalTex = CreateRenderTexture(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT);
	albedoTex = CreateRenderTexture(width, height, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	depthTex = CreateRenderTexture(width, height, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT);		
}

void GBuffer::Initialize(bool fat)
{
	GLenum attach = GL_COLOR_ATTACHMENT0;
	if (fat) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, attach, GL_TEXTURE_2D, positionTex, 0);
		++attach;
	}		
	glFramebufferTexture2D(GL_FRAMEBUFFER, attach, GL_TEXTURE_2D, normalTex, 0);
	++attach;
	glFramebufferTexture2D(GL_FRAMEBUFFER, attach, GL_TEXTURE_2D, albedoTex, 0);
	++attach;
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
}

void GBuffer::Destroy()
{
	glDeleteTextures(1, &depthTex);
	glDeleteTextures(1, &albedoTex);
	glDeleteTextures(1, &normalTex);
	glDeleteTextures(1, &positionTex);
	
	glDeleteFramebuffers(1, &FBO);
}

void GBuffer::Bind(bool fat)
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);	
	glViewport(0, 0, width, height);

	// Modifier les attachements peut-être plus rapide que de switcher les fbos
	Initialize(fat);

	if (fat) {
		static const GLenum mrt[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, mrt);
	}
	else {
		static const GLenum mrt[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, mrt);
	}	
}

void GBuffer::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
	glDrawBuffer(GL_BACK);
}

void GBuffer::DrawDebug(bool fat)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
	
	GLenum attach = GL_COLOR_ATTACHMENT0;

	if (fat) {
		// positions
		glReadBuffer(attach);
		glBlitFramebuffer(0, 0, width, height, 0, height / 2, width / 2, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		attach++;
	}
	// normales
	glReadBuffer(attach);
	glBlitFramebuffer(0, 0, width, height, width/2, height/2, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	attach++;
	// albedo (couleurs diffuses)
	glReadBuffer(attach);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width/2, height/2, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	attach++;
	// depth buffer
	glReadBuffer(GL_DEPTH_ATTACHMENT);
	glBlitFramebuffer(0, 0, width, height, width/2, 0, width, height/2, GL_DEPTH_BUFFER_BIT, GL_NEAREST);	
	Unbind();
}
