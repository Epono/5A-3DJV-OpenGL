//
// ESGI
// Particles Collision CPU
//


#include "Common.h"

#include <cstdio>
#include <cmath>

#include <vector>
#include <string>


#include "../common/EsgiShader.h"

#include "tinyobjloader/tiny_obj_loader.h"

#include "AntTweakBar.h"

#include "ParticleSystem.h"

// ---

TwBar* objTweakBar;

EsgiShader g_BasicShader;
EsgiShader g_SkyboxShader;

int previousTime = 0;

struct ViewProj
{
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
	glm::vec3 rotation;
	GLuint UBO;	
	bool autoRotateCamera;
} g_Camera;

struct Mesh
{
	GLuint VBO;
	GLuint IBO;
	GLuint ElementCount;
	GLenum PrimitiveType;
	GLuint VAO;
};

struct Objet
{
	// transform
	glm::vec3 position;
	glm::vec3 rotation;
	glm::mat4 worldMatrix;	
	// mesh
	Mesh* mesh;
	// material
	GLuint textureObj;
};

Mesh g_SphereMesh;
static const int g_NumSpheres = 3;
Objet g_Spheres[g_NumSpheres];

ParticleSystem g_ParticleSystem(ParticleBackEnd::CPU);

// ---

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
	glVertexAttribPointer(0, 3, GL_FLOAT, false, stride, nullptr);
	glEnableVertexAttribArray(0);
	if (normals.size()) {		
		glVertexAttribPointer(1, 3, GL_FLOAT, false, stride, (GLvoid *)offset);
		glEnableVertexAttribArray(1);
		offset += 3 * sizeof(float);
	}
	if (texcoords.size()) {
		glVertexAttribPointer(2, 2, GL_FLOAT, false, stride, (GLvoid *)offset);
		glEnableVertexAttribArray(2);
		offset += 2 * sizeof(float);
	}
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);	

	//LoadAndCreateTextureRGBA(materials[0].diffuse_texname.c_str(), g_Objet.textureObj);
}

void CleanOBJ(Mesh& mesh)
{
	//if (mesh.textureObj)
	//	glDeleteTextures(1, &mesh.textureObj);
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
	objTweakBar = TwNewBar("Particle Collision");
	TwAddVarRW(objTweakBar, "Auto Rotate Camera", TW_TYPE_BOOLCPP, &g_Camera.autoRotateCamera, "");
	TwAddSeparator(objTweakBar, "Objet", "");
	TwAddButton(objTweakBar, "Quitter", &ExitCallbackTw, nullptr, "");
	
	// Objets OpenGL

	g_BasicShader.LoadVertexShader("basic.vs");
	g_BasicShader.LoadFragmentShader("basic.fs");
	g_BasicShader.Create();

	glGenBuffers(1, &g_Camera.UBO);
	glBindBuffer(GL_UNIFORM_BUFFER, g_Camera.UBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, nullptr, GL_STREAM_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, g_Camera.UBO);
	//glBindBuffer(GL_UNIFORM_BUFFER, 0);

	auto program = g_BasicShader.GetProgram();

	auto blockIndex = glGetUniformBlockIndex(program, "ViewProj");
	glUniformBlockBinding(program, blockIndex, 0);

	// Setup

	previousTime = glutGet(GLUT_ELAPSED_TIME);

	const std::string inputFile = "sphere.obj";
	LoadOBJ(g_SphereMesh, inputFile);

	// Place N spheres de façon aléatoire
	Obstacle obstacle;
	const float big_radius = 20.0f;
	const float small_radius = 1.0f;
	glm::mat4& transform = g_Spheres[0].worldMatrix;
	transform = glm::mat4(1.0f);
	transform = glm::scale(transform, glm::vec3(big_radius));
	transform[3].y = big_radius * -1.25f;
	obstacle.position = glm::vec3(transform[3]);
	obstacle.radius = big_radius;
	g_ParticleSystem.m_Obstacles.push_back(obstacle);

	for (auto index = 1; index < g_NumSpheres; ++index) 
	{
		glm::mat4& transform = g_Spheres[index].worldMatrix;
		transform = glm::mat4(1.0f);
		float rnd = (rand() / (float)RAND_MAX) * 6.0f - 3.0f;
		transform[3] = glm::vec4(rnd, rnd, rnd, 1.0f);
		
		obstacle.position = glm::vec3(transform[3]);
		obstacle.radius = small_radius;
		g_ParticleSystem.m_Obstacles.push_back(obstacle);
	}

	g_ParticleSystem.Emit(1000*100);
}

void Terminate()
{		
	TwTerminate();

	g_ParticleSystem.Destroy();

	glDeleteBuffers(1, &g_Camera.UBO);
	
	CleanOBJ(g_SphereMesh);

	g_BasicShader.Destroy();
}

// boucle principale ---

void Resize(GLint width, GLint height) 
{
	glViewport(0, 0, width, height);
	
	g_Camera.projectionMatrix = glm::perspectiveFov(45.f, (float)width, (float)height, 0.1f, 1000.f);

	TwWindowSize(width, height);
}

static bool g_CanDraw = false;

void Update() 
{
	auto currentTime = glutGet(GLUT_ELAPSED_TIME);
	auto delta = currentTime - previousTime;
	previousTime = currentTime;
	auto elapsedTime = delta / 1000.0f;
	//g_Objet.rotation += glm::vec3(36.0f * elapsedTime);
	if (g_Camera.autoRotateCamera) {
		g_Camera.rotation.y += 10.f * elapsedTime;
	}
	
	g_ParticleSystem.Update(elapsedTime);

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
	const glm::vec4 orbitDistance(0.0f, 0.0f, 20.0f, 1.0f);
	glm::vec4 position = glm::eulerAngleY(rotY) * orbitDistance;
	g_Camera.viewMatrix = glm::lookAt(glm::vec3(position), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));

	glBindBuffer(GL_UNIFORM_BUFFER, g_Camera.UBO);
	//glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, glm::value_ptr(g_Camera.viewMatrix), GL_STREAM_DRAW);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4) * 2, glm::value_ptr(g_Camera.viewMatrix));


	// rendu	

	auto program = g_BasicShader.GetProgram();
	glUseProgram(program);
	
	auto worldLocation = glGetUniformLocation(program, "u_worldMatrix");		

	//glBindTexture(GL_TEXTURE_2D, g_Objet.textureObj);

	glBindVertexArray(g_SphereMesh.VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_SphereMesh.IBO);

	for (auto index = 0; index < g_NumSpheres; ++index) 
	{
		glm::mat4& transform = g_Spheres[index].worldMatrix;
		glUniformMatrix4fv(worldLocation, 1, GL_FALSE, glm::value_ptr(transform));

		glDrawElements(GL_TRIANGLES, g_SphereMesh.ElementCount, GL_UNSIGNED_INT, 0);
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	g_ParticleSystem.Render();

	//glBindTexture(GL_TEXTURE_2D, 0);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
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
	glutCreateWindow("OBJ Loader");

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

