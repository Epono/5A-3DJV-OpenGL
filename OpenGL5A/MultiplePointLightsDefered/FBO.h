#pragma once

#include "Common.h"

GLuint CreateRenderTexture(GLuint width, GLuint height, GLenum format, GLenum type) {
	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	// il est obligatoire de specifier une valeur pour GL_TEXTURE_MIN_FILTER
	// autrement le Texture Object est considere comme invalide
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, type, nullptr);

	return texID;
}

GLuint CreateFBO() {
	GLuint FBO;
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	return FBO;
}
