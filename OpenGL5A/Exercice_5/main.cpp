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
	// normals
	GLuint normals;
	bool hasNormals;
	// texcoords
	GLuint texcoords;
	bool hasTexcoords;
} g_Cube;

void Keyboard(unsigned char key, int x, int y) {
	switch(key) {
	case 27:
		exit(1);
	}
}

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
	std::string inputfile = "bear-obj.obj";
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err = tinyobj::LoadObj(shapes, materials, inputfile.c_str());
	const std::vector<unsigned int>& indices = shapes[0].mesh.indices;
	const std::vector<float>& positions = shapes[0].mesh.positions;
	const std::vector<float>& normals = shapes[0].mesh.normals;
	const std::vector<float>& texcoords = shapes[0].mesh.texcoords;

	g_Cube.hasNormals = normals.size() > 0;
	//g_Cube.hasTexcoords = texcoords.size() > 0;
	g_Cube.hasTexcoords = false;

	g_Cube.ElementCount = indices.size();

	// CODE A AJOUTER ICI POUR CHARGER LES INDICES ET ATTRIBUTS
	glGenBuffers(1, &g_Cube.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, g_Cube.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * positions.size(), &positions[0], GL_STATIC_DRAW);
	//GLvoid* vertexPointer = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
	//glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 5, vertexPointer);
	//glUnmapBuffer(GL_ARRAY_BUFFER);
	//GLvoid* PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	//memcpy(PositionBuffer, &positions, positions.size());
	//glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &g_Cube.IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_Cube.IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &g_Cube.VAO);
	glBindVertexArray(g_Cube.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, g_Cube.VBO);
	glBindVertexArray(0);

	if(g_Cube.hasNormals) {
		glGenBuffers(1, &g_Cube.normals);
		glBindBuffer(GL_ARRAY_BUFFER, g_Cube.normals);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * normals.size(), &normals[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	if(g_Cube.hasTexcoords) {
		glGenBuffers(1, &g_Cube.texcoords);
		glBindBuffer(GL_ARRAY_BUFFER, g_Cube.texcoords);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * texcoords.size(), &texcoords[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		LoadAndCreateTextureRGBA("Resources/texture.jpg", g_Cube.textureObj);
	}


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
		exit(-1);
		// hihi
	}

	for(int index = 0; index < numExtensions; ++index) {
		//printf("Extension[%d] : %s\n", index, glGetStringi(GL_EXTENSIONS, index));
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
	glGenBuffers(1, &g_Camera.UBO);
	glBindBuffer(GL_UNIFORM_BUFFER, g_Camera.UBO);
	// notez le GL_STREAM_DRAW pour indiquer que les donnees changent peu par frame 
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, NULL, GL_STREAM_DRAW);

	// Doit être inférieur à un certain nombre (lequel ?))
	GLuint bindingPoint = 0;
	glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, g_Camera.UBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	auto blockIndex = glGetUniformBlockIndex(program, "ViewProj");
	glUniformBlockBinding(program, blockIndex, bindingPoint);


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

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
	glBindBuffer(GL_UNIFORM_BUFFER, g_Camera.UBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, glm::value_ptr(g_Camera.viewMatrix), GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	float yaw = glm::radians(g_Cube.rotation.y);
	float pitch = glm::radians(g_Cube.rotation.x);
	float roll = glm::radians(g_Cube.rotation.z);
	g_Cube.worldMatrix = glm::eulerAngleYXZ(yaw, pitch, roll);

	auto worldLocation = glGetUniformLocation(program, "u_worldMatrix");
	auto colorLocation = glGetUniformLocation(program, "u_objectColor");

	// rendu

	//TRUC
	//GLvoid* vertexPointer = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
	//glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 5, nullptr);
	//glUnmapBuffer(GL_ARRAY_BUFFER);
	//

	// TODO : passer en glMapBuffer()
	glBindBuffer(GL_ARRAY_BUFFER, g_Cube.VBO);
	auto positionLocation = glGetAttribLocation(program, "a_position");
	glEnableVertexAttribArray(positionLocation);
	glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);

	auto hasNormalsLocation = glGetUniformLocation(program, "u_hasNormals");
	glUniform1i(hasNormalsLocation, g_Cube.hasNormals);
	if(g_Cube.hasNormals) {
		glBindBuffer(GL_ARRAY_BUFFER, g_Cube.normals);
		auto normalLocation = glGetAttribLocation(program, "a_normal");
		glEnableVertexAttribArray(normalLocation);
		glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);	
	}

	auto hasTexcoordsLocation = glGetUniformLocation(program, "u_hasTexcoords");
	glUniform1i(hasTexcoordsLocation, g_Cube.hasTexcoords);
	if(g_Cube.hasTexcoords) {
		glBindBuffer(GL_ARRAY_BUFFER, g_Cube.texcoords);
		auto texcoordsLocation = glGetAttribLocation(program, "a_texCoords");
		glEnableVertexAttribArray(texcoordsLocation);
		glVertexAttribPointer(texcoordsLocation, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_Cube.textureObj);
		GLint textureLocation = glGetUniformLocation(program, "u_texture0");
		glUniform1i(textureLocation, 0);
	}

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

				glDrawElements(GL_TRIANGLES, g_Cube.ElementCount, GL_UNSIGNED_INT, nullptr);
				//glDrawArrays(GL_TRIANGLES, 0, g_Cube.ElementCount);
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
	glutKeyboardFunc(Keyboard);

	glutMainLoop();

	Terminate();

	return 0;
}

