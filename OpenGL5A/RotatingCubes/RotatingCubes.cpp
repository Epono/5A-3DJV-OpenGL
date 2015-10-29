//
// ESGI
// RotatingCubes.cpp 
//

// Specifique a Windows
#if _WIN32
#define GLEW_STATIC 1
#include <GL/glew.h>
#include <gl/wglew.h>

#define FREEGLUT_LIB_PRAGMAS 0
#include "GL/freeglut.h"

#pragma comment(lib, "freeglut.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32s.lib")
#endif

#include <cstdio>
#include <cmath>

#include <vector>
#include <string>

#include "../common/EsgiShader.h"

// STB
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "tinyobjloader/tiny_obj_loader.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
// extension glm : 
#include "glm/gtx/euler_angles.hpp"

EsgiShader basicShader;

int previousTime = 0;

struct ViewProj {
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
	glm::vec3 rotation;
	GLuint UBO;
} g_Camera;

struct Objet {
	// transform
	glm::vec3 position;
	glm::vec3 rotation;
	glm::mat4 worldMatrix;
	// mesh
	GLuint VBO;
	GLuint IBO;
	GLuint ElementCount;
	GLenum PrimitiveType;
	GLuint VAO;
	// material
	GLuint textureObj;
} g_Cube;

// ---

bool LoadAndCreateTextureRGBA(const char *filename, GLuint &texID) {
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	// il est obligatoire de specifier une valeur pour GL_TEXTURE_MIN_FILTER
	// autrement le Texture Object est considere comme invalide
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	int w, h;
	uint8_t *data = stbi_load(filename, &w, &h, nullptr, STBI_rgb_alpha);
	if(data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		stbi_image_free(data);
	}
	return (data != nullptr);
}

//

void InitCube() {
	std::string inputfile = "cube02.obj";
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err = tinyobj::LoadObj(shapes, materials, inputfile.c_str());
	const std::vector<unsigned int>& indices = shapes[0].mesh.indices;
	const std::vector<float>& positions = shapes[0].mesh.positions;
	const std::vector<float>& normals = shapes[0].mesh.normals;
	const std::vector<float>& texcoords = shapes[0].mesh.texcoords;

	g_Cube.ElementCount = indices.size();

	// CODE A AJOUTER ICI POUR CHARGER LES INDICES ET ATTRIBUTS

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CleanCube() {
	glDeleteTextures(1, &g_Cube.textureObj);
	glDeleteVertexArrays(1, &g_Cube.VAO);
	glDeleteBuffers(1, &g_Cube.VBO);
	glDeleteBuffers(1, &g_Cube.IBO);
}

void Initialize() {
	printf("Version Pilote OpenGL : %s\n", glGetString(GL_VERSION));
	printf("Type de GPU : %s\n", glGetString(GL_RENDERER));
	printf("Fabricant : %s\n", glGetString(GL_VENDOR));
	printf("Version GLSL : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	int numExtensions;
	glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

	GLenum error = glewInit();
	if(error != GL_NO_ERROR) {
		// TODO
	}

	for(int index = 0; index < numExtensions; ++index) {
		printf("Extension[%d] : %s\n", index, glGetStringi(GL_EXTENSIONS, index));
	}

#ifdef _WIN32
	// on coupe la synchro vertical pour voir l'effet du delta time
	wglSwapIntervalEXT(0);
#endif

	basicShader.LoadVertexShader("basic.vs");
	basicShader.LoadFragmentShader("basic.fs");
	basicShader.Create();

	auto program = basicShader.GetProgram();

	// UN UBO SERAIT UTILE ICI

	previousTime = glutGet(GLUT_ELAPSED_TIME);

	InitCube();

	// render states par defaut
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
}

void Terminate() {
	glDeleteBuffers(1, &g_Camera.UBO);

	CleanCube();

	basicShader.Destroy();
}

// ---

void Resize(GLint width, GLint height) {
	glViewport(0, 0, width, height);

	g_Camera.projectionMatrix = glm::perspectiveFov(45.f, (float) width, (float) height, 0.1f, 1000.f);
}

void Update() {
	auto currentTime = glutGet(GLUT_ELAPSED_TIME);
	auto delta = currentTime - previousTime;
	previousTime = currentTime;
	auto elapsedTime = delta / 1000.0f;
	g_Cube.rotation += glm::vec3(36.0f * elapsedTime);

	g_Camera.rotation.y += 10.f * elapsedTime;

	glutPostRedisplay();
}

void Render() {
	auto width = glutGet(GLUT_WINDOW_WIDTH);
	auto height = glutGet(GLUT_WINDOW_HEIGHT);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	auto program = basicShader.GetProgram();
	glUseProgram(program);

	// variables uniformes (constantes) 

	g_Camera.projectionMatrix = glm::perspectiveFov(45.f, (float) width, (float) height, 0.1f, 1000.f);
	// rotation orbitale de la camera
	float rotY = glm::radians(g_Camera.rotation.y);
	glm::vec4 position = glm::eulerAngleY(rotY) * glm::vec4(0.0f, 0.0f, 80.0f, 1.0f);
	g_Camera.viewMatrix = glm::lookAt(glm::vec3(position), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));

	// IL FAUT TRANSFERER LES MATRICES VIEW ET PROJ AU SHADER

	float yaw = glm::radians(g_Cube.rotation.y);
	float pitch = glm::radians(g_Cube.rotation.x);
	float roll = glm::radians(g_Cube.rotation.z);
	g_Cube.worldMatrix = glm::eulerAngleYXZ(yaw, pitch, roll);

	auto worldLocation = glGetUniformLocation(program, "u_worldMatrix");
	auto colorLocation = glGetUniformLocation(program, "u_objectColor");

	// rendu

	glBindVertexArray(g_Cube.VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_Cube.IBO);

	for(float x = -24.f; x < 24.f; x += 3.0f) {
		for(float y = -24.f; y < 24.f; y += 3.0f) {
			for(float z = -24.f; z < 24.f; z += 3.0f) {
				glUniform4f(colorLocation, (x + 24.0f) / 48.f, (y + 24.0f) / 48.f, (z + 24.0f) / 48.f, 1.0f);
				//glm::mat4 transform = glm::translate(g_Cube.worldMatrix, glm::vec3(x, y, z));
				glm::mat4& transform = g_Cube.worldMatrix;
				transform[3] = glm::vec4(x, y, z, 1.0f);
				glUniformMatrix4fv(worldLocation, 1, GL_FALSE, glm::value_ptr(transform));

				glDrawElements(GL_TRIANGLES, g_Cube.ElementCount, GL_UNSIGNED_INT, 0);
			}
		}
	}

	glutSwapBuffers();
}

int main(int argc, char* argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	glutCreateWindow("Un gros cube, des petits cubes, c'est les Rotating Cubes");

#ifdef FREEGLUT
	// Note: glutSetOption n'est disponible qu'avec freeGLUT
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,
				  GLUT_ACTION_GLUTMAINLOOP_RETURNS);
#endif

	Initialize();

	glutReshapeFunc(Resize);
	glutIdleFunc(Update);
	glutDisplayFunc(Render);

	glutMainLoop();

	Terminate();

	return 0;
}

