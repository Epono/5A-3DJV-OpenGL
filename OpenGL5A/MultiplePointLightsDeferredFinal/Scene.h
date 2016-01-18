
#pragma once

#include "Common.h"
#include <algorithm>
#include <string>

// ---

struct Vertex
{
	glm::vec3 position;
	glm::vec2 uv;
	glm::vec3 normal;
};

struct Mesh
{
	GLuint VBO;
	GLuint IBO;
	GLuint ElementCount;
	GLenum PrimitiveType;
	GLuint VAO;
};

// ---

struct Walls
{
	glm::mat4 worldMatrix;

	static const int gWallTexture = 0;
	static const int gFloorTexture = 1;
	static const int gCeilingTexture = 2;
	GLuint textures[3];
};

struct PointLight
{
	glm::vec4 position;
	//float radius;
	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular;

	static GLuint UBO;	
};

struct Material
{
	glm::vec4 emissive;
	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular; // specular.w = shininess

	static GLuint UBO;
};


struct Sphere
{
	// transform
	glm::vec3 position;
	glm::vec3 velocity;
	glm::mat4 worldMatrix;	
	// mesh
	Mesh* mesh;

	void initialize(PointLight& light);
	void update(float dt);
	void setUniformScale(float scale);
};


// ---

#define ATTR_POSITION	0
#define ATTR_TEXCOORDS	1
#define ATTR_NORMAL		2

template<size_t N>
void LoadMesh(Mesh& mesh, const Vertex (& vertices)[N])
{
	const auto stride = 3 * sizeof(float) + 2 * sizeof(float) + 3 * sizeof(float);
	const auto count = N;
	const auto totalSize = count * stride;
	
	mesh.ElementCount = N;
	mesh.IBO = 0;

	glGenBuffers(1, &mesh.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
	glBufferData(GL_ARRAY_BUFFER, totalSize, &vertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &mesh.VAO);
	glBindVertexArray(mesh.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
	auto offset = 3 * sizeof(float);
	glVertexAttribPointer(ATTR_POSITION, 3, GL_FLOAT, false, stride, nullptr);
	glEnableVertexAttribArray(ATTR_POSITION);
	glVertexAttribPointer(ATTR_TEXCOORDS, 2, GL_FLOAT, false, stride, (GLvoid *)offset);
	glEnableVertexAttribArray(ATTR_TEXCOORDS);
	offset += 2 * sizeof(float);	
	glVertexAttribPointer(ATTR_NORMAL, 3, GL_FLOAT, false, stride, (GLvoid *)offset);
	glEnableVertexAttribArray(ATTR_NORMAL);
	offset += 3 * sizeof(float);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void LoadOBJ(Mesh& mesh, const std::string& inputFile);

void CleanMesh(Mesh& mesh);
