
#include "Scene.h"

#include "tinyobjloader/tiny_obj_loader.h"

#include <vector>

static const float ROOM_SIZE_X = 256.0f;
static const float ROOM_SIZE_Y = 128.0f;
static const float ROOM_SIZE_Z = 256.0f;
static const float ROOM_SIZE_X_HALF = ROOM_SIZE_X * 0.5f;
static const float ROOM_SIZE_Y_HALF = ROOM_SIZE_Y * 0.5f;
static const float ROOM_SIZE_Z_HALF = ROOM_SIZE_Z * 0.5f;

static const float ROOM_WALL_TILE_U = 4.0f;
static const float ROOM_WALL_TILE_V = 2.0f;
static const float ROOM_FLOOR_TILE_U = 4.0f;
static const float ROOM_FLOOR_TILE_V = 4.0f;
static const float ROOM_CEILING_TILE_U = 4.0f;
static const float ROOM_CEILING_TILE_V = 4.0f;

const float LIGHT_OBJECT_LAUNCH_ANGLE = 45.0f;
const float LIGHT_OBJECT_RADIUS = 2.0f;
const float LIGHT_OBJECT_SPEED = 80.0f;
const float LIGHT_RADIUS_MAX = std::max(std::max(ROOM_SIZE_X, ROOM_SIZE_Y), ROOM_SIZE_Z) * 1.25f;
const float LIGHT_RADIUS_MIN = 0.0f;

Vertex g_Room[36] =
{
    // Wall: +Z face
    {glm::vec3(-ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, 0.0f),									glm::vec3(0.0f,  0.0f, -1.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, 0.0f),						glm::vec3(0.0f,  0.0f, -1.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, ROOM_WALL_TILE_V),			glm::vec3(0.0f,  0.0f, -1.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, ROOM_WALL_TILE_V),			glm::vec3(0.0f,  0.0f, -1.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, ROOM_WALL_TILE_V),						glm::vec3(0.0f,  0.0f, -1.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, 0.0f),									glm::vec3(0.0f,  0.0f, -1.0f)},

    // Wall: -Z face
	{glm::vec3(ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),		glm::vec2(0.0f, 0.0f),									glm::vec3(0.0f,  0.0f,  1.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, 0.0f),						glm::vec3(0.0f,  0.0f,  1.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, ROOM_WALL_TILE_V),			glm::vec3(0.0f,  0.0f,  1.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, ROOM_WALL_TILE_V),			glm::vec3(0.0f,  0.0f,  1.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, ROOM_WALL_TILE_V),						glm::vec3(0.0f,  0.0f,  1.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, 0.0f),									glm::vec3(0.0f,  0.0f,  1.0f)},

    // Wall: -X face
    {glm::vec3(-ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, 0.0f),									glm::vec3(1.0f,  0.0f,  0.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, 0.0f),						glm::vec3(1.0f,  0.0f,  0.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, ROOM_WALL_TILE_V),			glm::vec3(1.0f,  0.0f,  0.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, ROOM_WALL_TILE_V),			glm::vec3(1.0f,  0.0f,  0.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, ROOM_WALL_TILE_V),						glm::vec3(1.0f,  0.0f,  0.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, 0.0f),									glm::vec3(1.0f,  0.0f,  0.0f)},

    // Wall: +X face
    {glm::vec3( ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, 0.0f),									glm::vec3(-1.0f,  0.0f,  0.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, 0.0f),						glm::vec3(-1.0f,  0.0f,  0.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, ROOM_WALL_TILE_V),			glm::vec3(-1.0f,  0.0f,  0.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, ROOM_WALL_TILE_V),			glm::vec3(-1.0f,  0.0f,  0.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, ROOM_WALL_TILE_V),						glm::vec3(-1.0f,  0.0f,  0.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, 0.0f),									glm::vec3(-1.0f,  0.0f,  0.0f)},

    // Ceiling: +Y face
    {glm::vec3(-ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, 0.0f),									glm::vec3(0.0f, -1.0f,  0.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_CEILING_TILE_U, 0.0f),					glm::vec3(0.0f, -1.0f,  0.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_CEILING_TILE_U, ROOM_CEILING_TILE_V),	glm::vec3(0.0f, -1.0f,  0.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_CEILING_TILE_U, ROOM_CEILING_TILE_V),	glm::vec3(0.0f, -1.0f,  0.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, ROOM_CEILING_TILE_V),					glm::vec3(0.0f, -1.0f,  0.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, 0.0f),									glm::vec3(0.0f, -1.0f,  0.0f)},

    // Floor: -Y face
    {glm::vec3(-ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, 0.0f),									glm::vec3(0.0f,  1.0f,  0.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_FLOOR_TILE_U, 0.0f),						glm::vec3(0.0f,  1.0f,  0.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_FLOOR_TILE_U, ROOM_FLOOR_TILE_V),		glm::vec3(0.0f,  1.0f,  0.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_FLOOR_TILE_U, ROOM_FLOOR_TILE_V),		glm::vec3(0.0f,  1.0f,  0.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, ROOM_FLOOR_TILE_V),						glm::vec3(0.0f,  1.0f,  0.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, 0.0f),									glm::vec3(0.0f,  1.0f,  0.0f)}
};


void Sphere::setUniformScale(float scale)
{
	// force une valeur de scale uniforme (diagonale)
	// ceci est un "hack" possible car on n'effectue aucune rotation
	// pour un resultat correct il faudrait reinitialiser la sous matrice 3x3 de worldMatrix
	// puis creer une matrice 4x4 que l'on va concatener avec worldMatrix
	worldMatrix[0].x = scale; worldMatrix[1].y = scale; worldMatrix[2].z = scale;
}

void Sphere::initialize(PointLight& light)
{
    // direction aleatoire calculee dans le repere polaire puis transposee dans le repere cartesien

    float rho = LIGHT_OBJECT_SPEED + 0.5f * (LIGHT_OBJECT_SPEED * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)));
	float phi = glm::radians(LIGHT_OBJECT_LAUNCH_ANGLE);
    float theta = glm::radians(360.0f * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)));

    velocity.x = rho * cosf(phi) * cosf(theta);
    velocity.y = rho * sinf(phi);
    velocity.z = rho * cosf(phi) * sinf(theta);

	glm::vec4 random(static_cast<float>(rand()) / static_cast<float>(RAND_MAX), static_cast<float>(rand()) / static_cast<float>(RAND_MAX), static_cast<float>(rand()) / static_cast<float>(RAND_MAX), 1.0f);	
	light.ambient = random;
	light.diffuse = random;
	light.specular = random;
	//light.radius = 100.0f;
}

void Sphere::update(float dt)
{
	position += velocity * dt;

    if (position.x > (ROOM_SIZE_X_HALF - LIGHT_OBJECT_RADIUS * 2.0f))
        velocity.x = -velocity.x;
    if (position.x < -(ROOM_SIZE_X_HALF - LIGHT_OBJECT_RADIUS * 2.0f))
        velocity.x = -velocity.x;

    if (position.y > (ROOM_SIZE_Y_HALF - LIGHT_OBJECT_RADIUS * 2.0f))
        velocity.y = -velocity.y;
    if (position.y < -(ROOM_SIZE_Y_HALF - LIGHT_OBJECT_RADIUS * 2.0f))
        velocity.y = -velocity.y;

    if (position.z > (ROOM_SIZE_Z_HALF - LIGHT_OBJECT_RADIUS * 2.0f))
        velocity.z = -velocity.z;
    if (position.z < -(ROOM_SIZE_Z_HALF - LIGHT_OBJECT_RADIUS * 2.0f))
        velocity.z = -velocity.z;

	worldMatrix[3].x = position.x;
	worldMatrix[3].y = position.y;
	worldMatrix[3].z = position.z;
}

void LoadOBJ(Mesh &mesh, const std::string &inputFile)
{	
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err = tinyobj::LoadObj(shapes, materials, inputFile.c_str());
	const std::vector<unsigned int>& indices = shapes[0].mesh.indices;
	const std::vector<float>& positions = shapes[0].mesh.positions;
	const std::vector<float>& normals = shapes[0].mesh.normals;
	const std::vector<float>& texcoords = shapes[0].mesh.texcoords;

	mesh.ElementCount = indices.size();
	
	uint32_t stride = 0;

	if (positions.size()) {
		stride += 3 * sizeof(float);
	}
	if (normals.size()) {
		stride += 3 * sizeof(float);
	}
	if (texcoords.size()) {
		stride += 2 * sizeof(float);
	}

	const auto count = positions.size() / 3;
	const auto totalSize = count * stride;
	
	glGenBuffers(1, &mesh.IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices[0], GL_STATIC_DRAW);
	
	glGenBuffers(1, &mesh.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
	glBufferData(GL_ARRAY_BUFFER, totalSize, nullptr, GL_STATIC_DRAW);

	// glMapBuffer retourne un pointeur sur la zone memoire allouee par glBufferData 
	// du Buffer Object qui est actuellement actif - via glBindBuffer(<cible>, <id>)
	// il est imperatif d'appeler glUnmapBuffer() une fois que l'on a termine car le
	// driver peut tres bien etre amener a modifier l'emplacement memoire du BO.
	float* vertices = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);		
	for (auto index = 0; index < count; ++index)
	{
		if (positions.size()) {
			memcpy(vertices, &positions[index * 3], 3 * sizeof(float));
			vertices += 3;
		}
		if (normals.size()) {
			memcpy(vertices, &normals[index * 3], 3 * sizeof(float));
			vertices += 3;
		}
		if (texcoords.size()) {
			memcpy(vertices, &texcoords[index * 2], 2 * sizeof(float));
			vertices += 2;
		}
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);

	glGenVertexArrays(1, &mesh.VAO);
	glBindVertexArray(mesh.VAO);
	if (mesh.ElementCount)
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.IBO);	
	glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
	uint32_t offset = 3 * sizeof(float);
	glVertexAttribPointer(ATTR_POSITION, 3, GL_FLOAT, false, stride, nullptr);
	glEnableVertexAttribArray(ATTR_POSITION);
	if (texcoords.size()) {
		glVertexAttribPointer(ATTR_TEXCOORDS, 2, GL_FLOAT, false, stride, (GLvoid *)offset);
		glEnableVertexAttribArray(ATTR_TEXCOORDS);
		offset += 2 * sizeof(float);
	}
	if (normals.size()) {		
		glVertexAttribPointer(ATTR_NORMAL, 3, GL_FLOAT, false, stride, (GLvoid *)offset);
		glEnableVertexAttribArray(ATTR_NORMAL);
		offset += 3 * sizeof(float);
	}
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void CleanMesh(Mesh& mesh)
{
	if (mesh.VAO)
		glDeleteVertexArrays(1, &mesh.VAO);
	if (mesh.VBO)
		glDeleteBuffers(1, &mesh.VBO);
	if (mesh.IBO)
		glDeleteBuffers(1, &mesh.IBO);
}


