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
} g_Camera;

struct Objet {
	glm::mat4 worldMatrix;
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

	glUseProgram(0);

#ifdef _WIN32
	wglSwapIntervalEXT(1);
#endif

	return true;
}

void Terminate() {
	g_BasicShader.Destroy();
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// alternativement on peut utiliser la nouvelle fonction glClearBufferxx()

	auto basicProgram = g_BasicShader.GetProgram();

	glUseProgram(basicProgram);

	g_Object.worldMatrix = glm::mat4(1.0f);

	g_Camera.projectionMatrix = glm::perspectiveFov(45.f, (float) glutGet(GLUT_WINDOW_WIDTH), (float) glutGet(GLUT_WINDOW_HEIGHT), 0.1f, 1000.f);
	glm::vec4 position = glm::vec4(0.0f, 0.0f, -5.0f, 1.0f);
	g_Camera.viewMatrix = glm::lookAt(glm::vec3(position), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));

	auto projLocation = glGetUniformLocation(basicProgram, "u_projectionMatrix");
	glUniformMatrix4fv(projLocation, 1, GL_FALSE, glm::value_ptr(g_Camera.projectionMatrix));

	auto viewLocation = glGetUniformLocation(basicProgram, "u_viewMatrix");
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(g_Camera.viewMatrix));

	auto worldLocation = glGetUniformLocation(basicProgram, "u_worldMatrix");
	glUniformMatrix4fv(worldLocation, 1, GL_FALSE, glm::value_ptr(g_Object.worldMatrix));

	static const float triangle[] = {
		-0.5f, -0.5f,
		0.5f, -0.5f,
		0.0f, 0.5f
	};

	// zero correspond ici a la valeur de layout(location=0) dans le shader basic.vs
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, triangle);

	GLint colorLocation = glGetUniformLocation(basicProgram, "u_color");
	GLint offsetLocation = glGetUniformLocation(basicProgram, "u_offset");

	//rand() % 10 - 4.5
	for(int i = 0; i < 10; ++i) {
		glUniform3f(offsetLocation, i - 4.5, 0.0f, 0.0f);
		glUniform4f(colorLocation, 0.0f, 0.0f, (float) i / 10, 0.0f);

		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

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