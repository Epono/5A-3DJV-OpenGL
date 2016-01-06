//
// ESGI
// Multiple Point Lights (forward rendering)
//


#include "Common.h"

#include <cstdio>
#include <cmath>
#include <algorithm>

#include <vector>
#include <string>


#include "../common/EsgiShader.h"

#include "tinyobjloader/tiny_obj_loader.h"

#include "AntTweakBar.h"

// ---

TwBar* objTweakBar;

EsgiShader g_AmbientShader;
EsgiShader g_BlinnPhongShader;

int previousTime = 0;

// ---

struct ViewProj
{
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
	glm::vec3 rotation;
	GLuint UBO;	
	bool autoRotateCamera;
} g_Camera;

// ---

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

#define MAX_POINT_LIGHTS	128
struct PointLight
{
	glm::vec4 position;
	//float radius;
	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular;

	static GLuint UBO;
	
} g_PointLights[MAX_POINT_LIGHTS];

GLuint PointLight::UBO;

struct Material
{
	glm::vec4 emissive;
	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular; // specular.w = shininess

	static GLuint UBO;
};

Material g_ShinyMaterial = {
    glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
    glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
    glm::vec4(1.0f, 1.0f, 1.0f, 32.0f)
};

GLuint Material::UBO;

// ---
struct Vertex
{
	glm::vec3 position;
	glm::vec2 uv;
	glm::vec3 normal;
};

Vertex g_Room[36] =
{
    // Wall: -Z face
	{glm::vec3(ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),		glm::vec2(0.0f, 0.0f),									glm::vec3(0.0f,  0.0f,  1.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, 0.0f),						glm::vec3(0.0f,  0.0f,  1.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, ROOM_WALL_TILE_V),			glm::vec3(0.0f,  0.0f,  1.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, ROOM_WALL_TILE_V),			glm::vec3(0.0f,  0.0f,  1.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, ROOM_WALL_TILE_V),						glm::vec3(0.0f,  0.0f,  1.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF, -ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, 0.0f),									glm::vec3(0.0f,  0.0f,  1.0f)},

    // Wall: +Z face
    {glm::vec3(-ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, 0.0f),									glm::vec3(0.0f,  0.0f, -1.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, 0.0f),						glm::vec3(0.0f,  0.0f, -1.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, ROOM_WALL_TILE_V),			glm::vec3(0.0f,  0.0f, -1.0f)},
    {glm::vec3( ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(ROOM_WALL_TILE_U, ROOM_WALL_TILE_V),			glm::vec3(0.0f,  0.0f, -1.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF, -ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, ROOM_WALL_TILE_V),						glm::vec3(0.0f,  0.0f, -1.0f)},
    {glm::vec3(-ROOM_SIZE_X_HALF,  ROOM_SIZE_Y_HALF,  ROOM_SIZE_Z_HALF),	glm::vec2(0.0f, 0.0f),									glm::vec3(0.0f,  0.0f, -1.0f)},

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

struct Mesh
{
	GLuint VBO;
	GLuint IBO;
	GLuint ElementCount;
	GLenum PrimitiveType;
	GLuint VAO;
};

struct Sphere
{
	// transform
	glm::vec3 position;
	glm::vec3 velocity;
	glm::mat4 worldMatrix;	
	// mesh
	Mesh* mesh;

	void initialize(uint32_t index);
	void update(float dt);
};

void Sphere::initialize(uint32_t index)
{
    // direction aleatoire calculee dans le repere polaire puis transposee dans le repere cartesien

    float rho = LIGHT_OBJECT_SPEED + 0.5f * (LIGHT_OBJECT_SPEED * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)));
	float phi = glm::radians(LIGHT_OBJECT_LAUNCH_ANGLE);
    float theta = glm::radians(360.0f * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)));

    velocity.x = rho * cosf(phi) * cosf(theta);
    velocity.y = rho * sinf(phi);
    velocity.z = rho * cosf(phi) * sinf(theta);

	//worldMatrix = glm::scale(worldMatrix, glm::vec3(0.1f));
	glm::vec4 random(static_cast<float>(rand()) / static_cast<float>(RAND_MAX), static_cast<float>(rand()) / static_cast<float>(RAND_MAX), static_cast<float>(rand()) / static_cast<float>(RAND_MAX), 1.0f);	
	g_PointLights[index].ambient = random;
	g_PointLights[index].diffuse = random;
	g_PointLights[index].specular = random;
	//g_PointLights[index].radius = 100.0f;
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
// ---

uint32_t g_NumPointLights = 1;

Mesh g_SphereMesh;
std::vector<Sphere> g_Spheres;

Mesh g_WallMesh;

struct Walls
{
	glm::mat4 worldMatrix;

	static const int gWallTexture = 0;
	static const int gFloorTexture = 1;
	static const int gCeilingTexture = 2;
	GLuint textures[3];
};
Walls g_Walls;

// ---
#define ATTR_POSITION	0
#define ATTR_TEXCOORDS	1
#define ATTR_NORMAL		2

template<size_t N>
void LoadMesh(Mesh &mesh, const Vertex (&vertices)[N])
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
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

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

// Initialisation et terminaison ---

static  void __stdcall ExitCallbackTw(void* clientData)
{
	glutLeaveMainLoop();
}

void Initialize()
{
	printf("Version Pilote OpenGL : %s\n", glGetString(GL_VERSION));
	printf("Type de GPU : %s\n", glGetString(GL_RENDERER));
	printf("Fabricant : %s\n", glGetString(GL_VENDOR));
	printf("Version GLSL : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	int numExtensions;
	glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
	
	GLenum error = glewInit();
	if (error != GL_NO_ERROR) {
		// TODO
	}

#if LIST_EXTENSIONS
	for (int index = 0; index < numExtensions; ++index)
	{
		printf("Extension[%d] : %s\n", index, glGetStringi(GL_EXTENSIONS, index));
	}
#endif
	
#ifdef _WIN32
	// on coupe la synchro vertical pour voir l'effet du delta time
	wglSwapIntervalEXT(0);
#endif

	// render states par defaut
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);	

	// AntTweakBar
	
	TwInit(TW_OPENGL, NULL); // ou TW_OPENGL_CORE selon le cas de figure
	objTweakBar = TwNewBar("Multiple Point Lights");
	TwAddVarRW(objTweakBar, "Num Point Lights", TW_TYPE_UINT32, &g_NumPointLights, "");	
	TwAddButton(objTweakBar, "Quitter", &ExitCallbackTw, nullptr, "");
	
	// Objets OpenGL

	glGenBuffers(1, &g_Camera.UBO);
	glBindBuffer(GL_UNIFORM_BUFFER, g_Camera.UBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, nullptr, GL_STREAM_DRAW);
	

	glGenBuffers(1, &PointLight::UBO);
	glBindBuffer(GL_UNIFORM_BUFFER, PointLight::UBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(PointLight) * MAX_POINT_LIGHTS, nullptr, GL_STREAM_DRAW);
	

	glGenBuffers(1, &Material::UBO);
	glBindBuffer(GL_UNIFORM_BUFFER, Material::UBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(Material), &g_ShinyMaterial, GL_STATIC_DRAW);
	
	error = glGetError(); assert(error == GL_NO_ERROR);

	g_AmbientShader.LoadVertexShader("ambient.vs");
	g_AmbientShader.LoadFragmentShader("ambient.fs");
	g_AmbientShader.Create();
	auto program = g_AmbientShader.GetProgram();

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, g_Camera.UBO);
	auto blockIndex = glGetUniformBlockIndex(program, "ViewProj");
	glUniformBlockBinding(program, blockIndex, 0);

	error = glGetError(); assert(error == GL_NO_ERROR);

	g_BlinnPhongShader.LoadVertexShader("blinnPhong.vs");
	g_BlinnPhongShader.LoadFragmentShader("blinnPhong.fs");
	g_BlinnPhongShader.Create();
	program = g_BlinnPhongShader.GetProgram();

	//glBindBufferBase(GL_UNIFORM_BUFFER, 0, g_Camera.UBO); // deja bound
	blockIndex = glGetUniformBlockIndex(program, "ViewProj");
	glUniformBlockBinding(program, blockIndex, 0);

	glBindBufferBase(GL_UNIFORM_BUFFER, 1, PointLight::UBO);
	blockIndex = glGetUniformBlockIndex(program, "Lights");
	glUniformBlockBinding(program, blockIndex, 1);

	glBindBufferBase(GL_UNIFORM_BUFFER, 2, Material::UBO);
	blockIndex = glGetUniformBlockIndex(program, "Material");
	glUniformBlockBinding(program, blockIndex, 2);

	// Setup
	error = glGetError(); assert(error == GL_NO_ERROR);

	previousTime = glutGet(GLUT_ELAPSED_TIME);

	LoadMesh(g_WallMesh, g_Room);

	LoadAndCreateTextureRGBA("wall_color_map.jpg", g_Walls.textures[Walls::gWallTexture]);
	glBindTexture(GL_TEXTURE_2D, g_Walls.textures[Walls::gWallTexture]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	LoadAndCreateTextureRGBA("floor_color_map.jpg", g_Walls.textures[Walls::gFloorTexture]);
	glBindTexture(GL_TEXTURE_2D, g_Walls.textures[Walls::gFloorTexture]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	LoadAndCreateTextureRGBA("ceiling_color_map.jpg", g_Walls.textures[Walls::gCeilingTexture]);
	glBindTexture(GL_TEXTURE_2D, g_Walls.textures[Walls::gCeilingTexture]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	const std::string inputFile = "sphere.obj";
	LoadOBJ(g_SphereMesh, inputFile);

	g_Spheres.resize(g_NumPointLights);
	for (uint32_t index = 0; index < g_NumPointLights; ++index)
	{
		g_Spheres[index].initialize(index);
	}

	error = glGetError(); assert(error == GL_NO_ERROR);
}

void Terminate()
{		
	TwTerminate();

	glDeleteBuffers(1, &Material::UBO);
	glDeleteBuffers(1, &PointLight::UBO);
	glDeleteBuffers(1, &g_Camera.UBO);

	g_Spheres.shrink_to_fit();
	
	CleanMesh(g_SphereMesh);

	glDeleteTextures(3, g_Walls.textures);
	CleanMesh(g_WallMesh);

	g_BlinnPhongShader.Destroy();
	g_AmbientShader.Destroy();
}

// boucle principale ---

void Resize(GLint width, GLint height) 
{
	glViewport(0, 0, width, height);
	
	g_Camera.projectionMatrix = glm::perspectiveFov(45.f, (float)width, (float)height, 0.1f, 1000.f);

	TwWindowSize(width, height);

	GLenum error = glGetError(); assert(error == GL_NO_ERROR);

}

static bool g_CanDraw = false;

void Update() 
{
	auto currentTime = glutGet(GLUT_ELAPSED_TIME);
	auto delta = currentTime - previousTime;
	previousTime = currentTime;
	auto elapsedTime = delta / 1000.0f;
	
	if (g_Camera.autoRotateCamera) {
		g_Camera.rotation.y += 10.f * elapsedTime;
	}

	if (g_Spheres.size() < g_NumPointLights)
	{
		for (auto index = g_Spheres.size(); index < g_NumPointLights; ++index) {
			g_Spheres.push_back(Sphere()); // pas terrible, cf cours C++ avance
			g_Spheres[index].initialize(index);
		}
	}

	for (uint32_t index = 0; index < g_NumPointLights; ++index)
	{
		g_Spheres[index].update(elapsedTime);
	}

	glutPostRedisplay();

	g_CanDraw = true;
}

void Render()
{
	if (!g_CanDraw)
		return;

	auto width = glutGet(GLUT_WINDOW_WIDTH);
	auto height = glutGet(GLUT_WINDOW_HEIGHT);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// variables uniformes (constantes) 

	g_Camera.projectionMatrix = glm::perspectiveFov(45.f, (float)width, (float)height, 0.1f, 1000.f);
	// rotation orbitale de la camera
	float rotY = glm::radians(g_Camera.rotation.y);
	const glm::vec4 orbitDistance(0.0f, 0.0f, 200.0f, 1.0f);
	glm::vec4 position = /*glm::eulerAngleY(rotY) **/ orbitDistance;
	g_Camera.viewMatrix = glm::lookAt(glm::vec3(position), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));

	glBindBuffer(GL_UNIFORM_BUFFER, g_Camera.UBO);
	//glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, glm::value_ptr(g_Camera.viewMatrix), GL_STREAM_DRAW);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4) * 2, glm::value_ptr(g_Camera.viewMatrix));

	for (uint32_t index = 0; index < g_Spheres.size(); ++index)
	{
		static const float radius = 100.0f;
		// L'illumination s'effectue dans le repere de la camera, il faut donc que les positions des lumieres
		// soient egalement exprimees dans le meme repere (view space)
		g_PointLights[index].position = g_Camera.viewMatrix * glm::vec4(g_Spheres[index].position, 1.0f);
		g_PointLights[index].position.w = radius;
	}
	glBindBuffer(GL_UNIFORM_BUFFER, PointLight::UBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PointLight) * g_NumPointLights, g_PointLights);

	// rendu des murs avec illumination	

	glBindVertexArray(g_WallMesh.VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	auto program = g_BlinnPhongShader.GetProgram();
	glUseProgram(program);
	
	auto numLightsLocation = glGetUniformLocation(program, "u_numLights");
	glUniform1i(numLightsLocation, g_NumPointLights);
	auto worldLocation = glGetUniformLocation(program, "u_worldMatrix");
	glm::mat4& transform = g_Walls.worldMatrix;
	glUniformMatrix4fv(worldLocation, 1, GL_FALSE, glm::value_ptr(transform));
	auto startIndex = 0;
	glBindTexture(GL_TEXTURE_2D, g_Walls.textures[Walls::gWallTexture]);	
	glDrawArrays(GL_TRIANGLES, startIndex, 6 * 4); startIndex += 6 * 4;	// 4 murs
	glBindTexture(GL_TEXTURE_2D, g_Walls.textures[Walls::gCeilingTexture]);	
	glDrawArrays(GL_TRIANGLES, startIndex, 6); startIndex += 6;	// plafond
	glBindTexture(GL_TEXTURE_2D, g_Walls.textures[Walls::gFloorTexture]);	
	glDrawArrays(GL_TRIANGLES, startIndex, 6); startIndex += 6;	// sol

	// rendu debug

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindVertexArray(g_SphereMesh.VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_SphereMesh.IBO);

	program = g_AmbientShader.GetProgram();
	glUseProgram(program);

	worldLocation = glGetUniformLocation(program, "u_worldMatrix");

	for (auto index = 0; index < g_Spheres.size(); ++index) 
	{
		glm::mat4& transform = g_Spheres[index].worldMatrix;
		glUniformMatrix4fv(worldLocation, 1, GL_FALSE, glm::value_ptr(transform));
		glDrawElements(GL_TRIANGLES, g_SphereMesh.ElementCount, GL_UNSIGNED_INT, 0);
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	//glBindVertexArray(0);

	// dessine les tweakBar
	TwDraw();  

	glutSwapBuffers();
}

int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	glutCreateWindow("Multiple Point Lights");

#ifdef FREEGLUT
	// Note: glutSetOption n'est disponible qu'avec freeGLUT
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,
				  GLUT_ACTION_GLUTMAINLOOP_RETURNS);
#endif

	Initialize();

	glutReshapeFunc(Resize);
	glutIdleFunc(Update);
	glutDisplayFunc(Render);

	// redirection pour AntTweakBar
	// dans le cas ou vous utiliseriez deja ces callbacks
	// il suffit d'appeler l'event d'AntTweakBar depuis votre fonction de rappel
	glutMouseFunc((GLUTmousebuttonfun)TwEventMouseButtonGLUT);
	glutMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);
	glutPassiveMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);
	glutKeyboardFunc((GLUTkeyboardfun)TwEventKeyboardGLUT);
	glutSpecialFunc((GLUTspecialfun)TwEventSpecialGLUT);
	TwGLUTModifiersFunc(glutGetModifiers);

	glutMainLoop();

	Terminate();

	return 0;
}

