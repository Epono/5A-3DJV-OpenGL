//
// ESGI
// Triangle Basique
//

// Specifique a Windows
#ifdef _WIN32
#define GLEW_STATIC 1
#include <gl/glew.h>
#include <gl/wglew.h>

#define FREEGLUT_LIB_PRAGMAS 0
#include <gl/freeglut.h>

#pragma comment(lib, "glew32s.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "freeglut.lib")
#endif

#include <cstdint>
#include <cstdio>
#include "../common/EsgiShader.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

struct Camera {
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
	GLuint UBO;
} g_Camera;

struct Objet {
	glm::mat4 worldMatrix;
	GLuint VBO;
	//GLuint EBO;
	GLuint VAO;
} g_Object;

EsgiShader g_BasicShader;

void Keyboard(unsigned char key, int x, int y) {
	switch(key) {
	case 27:
		exit(1);
	}
}

bool Initialise() {
	printf("Version GL : %s\n", glGetString(GL_VERSION));
	printf("Pilotes GL : %s\n", glGetString(GL_RENDERER));
	printf("Fabricant : %s\n", glGetString(GL_VENDOR));
	printf("Version GLSL : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	GLenum status = glewInit();

	g_BasicShader.LoadVertexShader("basic.vs");
	g_BasicShader.LoadFragmentShader("basic.fs");
	g_BasicShader.Create();

	auto basicProgram = g_BasicShader.GetProgram();
	glUseProgram(basicProgram);

	static const float triangle[] = {
		-0.5f, -0.5f,
		0.5f, -0.5f,
		0.0f, 0.5f
	};

	glGenBuffers(1, &g_Object.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, g_Object.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//static const unsigned short indexesTriangle[] = {0, 1, 2};

	//glGenBuffers(1, &g_Object.EBO);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_Object.EBO);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexesTriangle), indexesTriangle, GL_STATIC_DRAW);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &g_Object.VAO);
	glBindVertexArray(g_Object.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, g_Object.VBO);
	glEnableVertexAttribArray(0);
	// glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 3, nullptr); 
	// tres important ! toujours desactiver les VAO lorsque l’on ne s’en sert plus 
	glBindVertexArray(0);

	glGenBuffers(1, &g_Camera.UBO);
	glBindBuffer(GL_UNIFORM_BUFFER, g_Camera.UBO);
	// notez le GL_STREAM_DRAW pour indiquer que les donnees changent peu par frame 
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, NULL, GL_STREAM_DRAW);

	// Doit être inférieur à un certain nombre (lequel ?))
	GLuint bindingPoint = 0;
	glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, g_Camera.UBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	auto blockIndex = glGetUniformBlockIndex(basicProgram, "ViewProj");
	glUniformBlockBinding(basicProgram, blockIndex, bindingPoint);

	glUseProgram(0);

#ifdef _WIN32
	wglSwapIntervalEXT(1);
#endif

	return true;
}

void Terminate() {
	g_BasicShader.Destroy();
	glDeleteBuffers(1, &g_Object.VBO);
	//glDeleteBuffers(1, &g_Object.EBO);
	glDeleteVertexArrays(1, &g_Object.VAO);
}

void Resize(GLint width, GLint height) {
	glViewport(0, 0, width, height);

	g_Camera.projectionMatrix = glm::perspectiveFov(45.f, (float) width, (float) height, 0.1f, 1000.f);
}

void Update() {
	glutPostRedisplay();
}

void Render() {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// alternativement on peut utiliser la nouvelle fonction glClearBufferxx()

	auto basicProgram = g_BasicShader.GetProgram();

	glUseProgram(basicProgram);

	//glBindBuffer(GL_ARRAY_BUFFER, g_Object.VBO);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_Object.EBO);
	glBindVertexArray(g_Object.VAO);

	g_Object.worldMatrix = glm::mat4(1.0f);

	g_Camera.projectionMatrix = glm::perspectiveFov(45.f, (float) glutGet(GLUT_WINDOW_WIDTH), (float) glutGet(GLUT_WINDOW_HEIGHT), 0.1f, 1000.f);
	glm::vec4 position = glm::vec4(0.0f, 0.0f, -5.0f, 1.0f);
	g_Camera.viewMatrix = glm::lookAt(glm::vec3(position), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));

	glBindBuffer(GL_UNIFORM_BUFFER, g_Camera.UBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, glm::value_ptr(g_Camera.viewMatrix), GL_STREAM_DRAW);
	// on peut aussi utiliser glBufferSubData, pas forcement plus optimal 
	//glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4) * 2, glm::value_ptr(g_Camera.viewMatrix));

	auto worldLocation = glGetUniformLocation(basicProgram, "u_worldMatrix");
	glUniformMatrix4fv(worldLocation, 1, GL_FALSE, glm::value_ptr(g_Object.worldMatrix));

	GLint colorLocation = glGetUniformLocation(basicProgram, "u_color");
	GLint offsetLocation = glGetUniformLocation(basicProgram, "u_offset");

	// zero correspond ici a la valeur de layout(location=0) dans le shader basic.vs
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 3, nullptr);
	glEnableVertexAttribArray(0);

	//rand() % 10 - 4.5
	for(int i = 0; i < 10; ++i) {
		glUniform3f(offsetLocation, i - 4.5, 0.0f, 0.0f);
		glUniform4f(colorLocation, 0.0f, 0.0f, (float) i / 10, 0.0f);

		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	glBindVertexArray(0);
	glutSwapBuffers();
}

int main(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(1280, 720);
	glutCreateWindow("Triangle");

	Initialise();

	glutReshapeFunc(Resize);
	glutIdleFunc(Update);
	glutDisplayFunc(Render);
	glutKeyboardFunc(Keyboard);
#if FREEGLUT
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
#endif

	glutMainLoop();

	Terminate();

	return 1;
}