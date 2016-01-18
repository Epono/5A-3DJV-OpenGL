
#pragma once

#include "Common.h"


struct GBuffer
{
	GLuint width, height;

	GLuint FBO;
	GLuint positionTex;
	GLuint normalTex;
	GLuint albedoTex;
	GLuint depthTex;

	void Create();
	void Initialize(bool fat);
	void Bind(bool fat);
	void Unbind();
	void Destroy();
	void DrawDebug(bool fat);
};
