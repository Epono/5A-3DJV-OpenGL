//
// ESGI
// ObjLoader.cpp 
//


#include "Common.h"

#include <cstdio>
#include <cmath>

#include <vector>
#include <string>


#include "../common/EsgiShader.h"

#include "tinyobjloader/tiny_obj_loader.h"

#include "AntTweakBar.h"

#include "dds.h"

// ---

TwBar* objTweakBar;

EsgiShader g_TextureSSQuadShader;
EsgiShader g_BasicShader;
EsgiShader g_SkyboxShader;
EsgiShader g_EnvMapShader;
EsgiShader g_LocalEnvMapShader;

bool g_EnableLocalReflections = false;
int previousTime = 0;

struct ViewProj
{
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
	glm::vec3 rotation;
	GLuint UBO;	
	bool autoRotateCamera;
} g_Camera;

struct Objet
{
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
};

GLuint g_defaultTexId;

GLuint g_CubeMapDynamicTextureID;
GLuint g_CubeMapDepthBuffer;
GLuint g_CubeMapFBO;


Objet g_Cubes[6];
Objet g_CubeMap;

Objet g_Sphere;

// ---

void InitCubeMap()
{
	static const float skyboxVertices[] = {        
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
  
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
  
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
   
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
  
        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
  
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

	static const char* skyboxFiles[] = {
		"skybox/right.jpg", 
		"skybox/left.jpg",
		"skybox/top.jpg",
		"skybox/bottom.jpg",
		"skybox/back.jpg",
		"skybox/front.jpg",
	};

	LoadAndCreateCubeMap(skyboxFiles, g_CubeMap.textureObj);

	glGenVertexArrays(1, &g_CubeMap.VAO);
    glGenBuffers(1, &g_CubeMap.VBO);
    glBindVertexArray(g_CubeMap.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_CubeMap.VBO);
	// RAPPEL: sizeof(skyboxVertices) fonctionne -cad qu'on obtient la taille totale du tableau-
	//			du fait que skyboxVertices est un tableau STATIC et donc que le compilateur peut deduire
	//			sa taille lors de la compilation. Autrement sizeof aurait du renvoyer la taille du pointeur.
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glBindVertexArray(0);

	g_SkyboxShader.LoadVertexShader("skybox.vs");
	g_SkyboxShader.LoadFragmentShader("skybox.fs");
	g_SkyboxShader.Create();

	auto program = g_SkyboxShader.GetProgram();
	glUseProgram(program);
	// l'UBO binde en 0 sera egalement connecte au shader skybox
	auto blockIndex = glGetUniformBlockIndex(program, "ViewProj");
	glUniformBlockBinding(program, blockIndex, 0);
	auto cubemapIndex = glGetUniformLocation(program, "u_cubeMap");
	glUniform1i(cubemapIndex, 1);
	
	// Shaders d'environment mapping

	g_EnvMapShader.LoadVertexShader("envMap.vs");
	g_EnvMapShader.LoadFragmentShader("envMap.fs");
	g_EnvMapShader.Create();

	program = g_EnvMapShader.GetProgram();
	glUseProgram(program);
	// l'UBO binde en 0 sera egalement connecte au shader envMap
	blockIndex = glGetUniformBlockIndex(program, "ViewProj");
	glUniformBlockBinding(program, blockIndex, 0);
	auto dynamicCubeMapIndex = glGetUniformLocation(program, "u_dynamicCubeMap");
	cubemapIndex = glGetUniformLocation(program, "u_cubeMap");
	glUniform1i(dynamicCubeMapIndex, 0);
	glUniform1i(cubemapIndex, 1);

	// techniquement il s'agit du meme vertex shader...
	g_LocalEnvMapShader.LoadVertexShader("localEnvMap.vs");
	g_LocalEnvMapShader.LoadFragmentShader("localEnvMap.fs");
	g_LocalEnvMapShader.Create();

	program = g_LocalEnvMapShader.GetProgram();
	glUseProgram(program);
	// l'UBO binde en 0 sera egalement connecte au shader envMap
	blockIndex = glGetUniformBlockIndex(program, "ViewProj");
	glUniformBlockBinding(program, blockIndex, 0);
	dynamicCubeMapIndex = glGetUniformLocation(program, "u_dynamicCubeMap");
	cubemapIndex = glGetUniformLocation(program, "u_cubeMap");
	glUniform1i(dynamicCubeMapIndex, 0);
	glUniform1i(cubemapIndex, 1);

	glUseProgram(0);

	// cubemap FBO

	auto cubeMapWidth = 1024, cubemapHeight = 1024;

	glGenTextures(1, &g_CubeMapDynamicTextureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, g_CubeMapDynamicTextureID);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	for(int faceIndex = 0; faceIndex < 6; ++faceIndex)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, 0, GL_RGBA8, cubeMapWidth, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	}

	glGenFramebuffers(1, &g_CubeMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, g_CubeMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, g_CubeMapDynamicTextureID, 0);
	glGenRenderbuffers(1, &g_CubeMapDepthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, g_CubeMapDepthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, cubeMapWidth, 1024); 
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g_CubeMapDepthBuffer);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DestroyCubeMap()
{		
	glDeleteTextures(1, &g_CubeMapDynamicTextureID);
	glDeleteRenderbuffers(1, &g_CubeMapDepthBuffer);
	glDeleteFramebuffers(1, &g_CubeMapFBO);
}

void LoadOBJ(const std::string& inputFile, Objet& objet)
{	
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err = tinyobj::LoadObj(shapes, materials, inputFile.c_str());
	const std::vector<unsigned int>& indices = shapes[0].mesh.indices;
	const std::vector<float>& positions = shapes[0].mesh.positions;
	const std::vector<float>& normals = shapes[0].mesh.normals;
	const std::vector<float>& texcoords = shapes[0].mesh.texcoords;

	objet.ElementCount = indices.size();
	
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

	glGenVertexArrays(1, &objet.VAO);
	glBindVertexArray(objet.VAO);

	if (objet.ElementCount) {
		glGenBuffers(1, &objet.IBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objet.IBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices[0], GL_STATIC_DRAW);
	}

	glGenBuffers(1, &objet.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, objet.VBO);
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
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	//LoadAndCreateTextureRGBA(materials[0].diffuse_texname.c_str(), g_Objet.textureObj);
	//LoadAndCreateTextureDXT("Rock-Texture-Surface.dds", g_Objet.textureObj);
}

void CleanObjet(Objet& objet)
{
	if (objet.textureObj)
		glDeleteTextures(1, &objet.textureObj);
	if (objet.VAO)
		glDeleteVertexArrays(1, &objet.VAO);
	if (objet.VBO)
		glDeleteBuffers(1, &objet.VBO);
	if (objet.IBO)
		glDeleteBuffers(1, &objet.IBO);
}

// Initialisation et terminaison ---

static void __stdcall ToggleLocalReflectionsTw(void* clientData)
{
	*((bool *)clientData) ^= true;
}

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
	objTweakBar = TwNewBar("OBJ Loader");
	TwAddSeparator(objTweakBar, "Camera", "");
	TwAddVarRW(objTweakBar, "Auto Rotate Camera", TW_TYPE_BOOLCPP, &g_Camera.autoRotateCamera, "");
	TwAddSeparator(objTweakBar, "Objet", "");
	TwAddButton(objTweakBar, "Reflections Locales", &ToggleLocalReflectionsTw, &g_EnableLocalReflections, "");
	TwAddSeparator(objTweakBar, "...", "");
	TwAddButton(objTweakBar, "Quitter", &ExitCallbackTw, nullptr, "");
	
	// Objets OpenGL
#ifdef CLEAR_CUBEMAP_WITH_SKYBOX
	g_TextureSSQuadShader.LoadVertexShader("textureSS.vs");
	g_TextureSSQuadShader.LoadFragmentShader("textureSS.fs");
	g_TextureSSQuadShader.Create();
#endif

	g_BasicShader.LoadVertexShader("basic.vs");
	g_BasicShader.LoadFragmentShader("basic.fs");
	g_BasicShader.Create();

	glGenBuffers(1, &g_Camera.UBO);
	glBindBuffer(GL_UNIFORM_BUFFER, g_Camera.UBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, nullptr, GL_STREAM_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, g_Camera.UBO);
	//glBindBuffer(GL_UNIFORM_BUFFER, 0);
	
	auto program =  g_BasicShader.GetProgram();
	glUseProgram(program);
	auto samplerIndex = glGetUniformLocation(program, "u_sampler");
	glUniform1i(samplerIndex, 0);
	
	auto blockIndex = glGetUniformBlockIndex(program, "ViewProj");
	glUniformBlockBinding(program, blockIndex, 0);

	// Setup

	previousTime = glutGet(GLUT_ELAPSED_TIME);

	//CreateDefaultTexture(g_defaultTexId);

	const std::string inputFile0 = "sphere.obj";
	LoadOBJ(inputFile0, g_Sphere);

	const std::string inputFile1 = "cube03.obj";
	LoadOBJ(inputFile1, g_Cubes[0]);

	g_Cubes[0].position = glm::vec3(0.0f, 0.0f, -2.0f);
	g_Cubes[1].position = glm::vec3(0.0f, 0.0f, +2.0f);
	g_Cubes[2].position = glm::vec3(0.0f, -2.0f, 0.0f);
	g_Cubes[3].position = glm::vec3(0.0f, +2.0f, 0.0f);
	g_Cubes[4].position = glm::vec3(-2.0f, 0.0f, 0.0f);
	g_Cubes[5].position = glm::vec3(+2.0f, 0.0f, 0.0f);

	InitCubeMap();	


}

void Terminate()
{	
	DestroyCubeMap();

	glDeleteTextures(1, &g_defaultTexId);
	glDeleteBuffers(1, &g_Camera.UBO);
	
	CleanObjet(g_Cubes[0]);
	CleanObjet(g_Sphere);
	CleanObjet(g_CubeMap);

	g_TextureSSQuadShader.Destroy();
	g_EnvMapShader.Destroy();
	g_SkyboxShader.Destroy();
	g_BasicShader.Destroy();

	TwTerminate();
}

// boucle principale ---

void Resize(GLint width, GLint height) 
{
	glViewport(0, 0, width, height);
	
	g_Camera.projectionMatrix = glm::perspectiveFov(45.f, (float)width, (float)height, 0.1f, 1000.f);

	TwWindowSize(width, height);
}

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

	g_Cubes[0].rotation += glm::vec3(5.0f, 15.f, 2.5f) * elapsedTime;

	g_Sphere.worldMatrix[3].x = 3.0f * sin(currentTime/1000.f);

	static char title[256];
	static float timer = 0.1f;
	timer += elapsedTime;
	if (timer >= 0.1f) {
		sprintf(title, "%s (%3.2f ms approx. %4.2f fps)", "Obj Loader - Dynamic Environment Mapping", elapsedTime*1000.f, 1.0f / elapsedTime);
		glutSetWindowTitle(title);
		timer -= 0.1f;
	}

	glutPostRedisplay();
}


void RenderCubes()
{
	// cubes
	
	auto program = g_BasicShader.GetProgram();
	glUseProgram(program);
	
	auto worldLocation = glGetUniformLocation(program, "u_worldMatrix");		

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_defaultTexId);
	
	glBindVertexArray(g_Cubes[0].VAO);

	for (auto i = 0; i < 6; ++i) {
		glm::mat4& transform = g_Cubes[0].worldMatrix;
		transform[3] = glm::vec4(g_Cubes[i].position, 1.0f);
		glm::vec4 color = glm::vec4((g_Cubes[i].position / 4.0f) + 0.5f, 1.0f); 
		glUniform4fv(glGetUniformLocation(program, "u_diffuseColor"), 1, glm::value_ptr(color));
		glUniformMatrix4fv(worldLocation, 1, GL_FALSE, glm::value_ptr(transform));

		glDrawElements(GL_TRIANGLES, g_Cubes[0].ElementCount, GL_UNSIGNED_INT, 0);
	}
}

// 

void RenderToCubeMap()
{
	const auto size = 1024.f;

	GLenum error = glGetError(); assert(error == GL_NO_ERROR);
	glBindFramebuffer(GL_FRAMEBUFFER, g_CubeMapFBO);
	glViewport(0, 0, size, size);
	glClearColor(0.0, 0.0, 0.0, 0.0);	
	
	static const glm::vec3 cubeMapDir[] = { glm::vec3(1.f, 0.f, 0.f), glm::vec3(-1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 0.f, -1.f) };
	static const glm::vec3 cubeMapUp[] = { glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 1.f, 0.f) };	

#if CLEAR_CUBEMAP_FACE_WITH_SKYBOX
	// procedure assez complexe 
	auto program = g_TextureSSQuadShader.GetProgram();
	glUseProgram(program);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, g_CubeMap.textureObj);
	glUniform1i(glGetUniformLocation(program, "u_dynamicCubeMap"), 1);

	const auto invSize = 1.0f / size;
	const auto invSizeBy2 = 2.0f / size;

	static bool firstTime = true;
	static GLuint VBO[2];
	static GLuint VAO;
	if (firstTime)
	{
	//float cubeMapTexCoords[] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
	static const float quad[] = {
	-1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f
	};

	//const glm::vec3 cubeMapTexCoords[] = { glm::normalize(glm::vec3(1.0f)), glm::normalize(glm::vec3(1.0f)), glm::normalize(glm::vec3(1.0f)), glm::normalize(glm::vec3(1.0f)) };
	const float minOffset = (invSizeBy2 * 0.0f + invSize), maxOffset = (invSizeBy2 * size + invSize);
	static const glm::vec3 cubeMapTexCoords[] = { 
												// POSITIVE X
												glm::vec3( 1.0f, 1.0f - maxOffset, 1.0f - minOffset)
												, glm::vec3( 1.0f, 1.0f - minOffset, 1.0f - minOffset)												
												, glm::vec3( 1.0f, 1.0f - maxOffset, 1.0f - maxOffset)
												, glm::vec3( 1.0f, 1.0f - minOffset, 1.0f - maxOffset)
												// NEGATIVE X
												, glm::vec3(-1.0f, 1.0f - maxOffset, -1.0f + minOffset)
												, glm::vec3(-1.0f, 1.0f - minOffset, -1.0f + minOffset)												
												, glm::vec3(-1.0f, 1.0f - maxOffset, -1.0f + maxOffset)
												, glm::vec3(-1.0f, 1.0f - minOffset, -1.0f + maxOffset)
												// POSITIVE Y
												, glm::vec3(-1.0f + minOffset, 1.0f, -1.0f + maxOffset)
												, glm::vec3(-1.0f + minOffset, 1.0f, -1.0f + minOffset)												
												, glm::vec3(-1.0f + maxOffset, 1.0f, -1.0f + maxOffset)
												, glm::vec3(-1.0f + maxOffset, 1.0f, -1.0f + minOffset)
												// NEGATIVE Y
												, glm::vec3(-1.0f + minOffset, -1.0f, 1.0f - maxOffset)
												, glm::vec3(-1.0f + minOffset, -1.0f, 1.0f - minOffset)												
												, glm::vec3(-1.0f + maxOffset, -1.0f, 1.0f - maxOffset)
												, glm::vec3(-1.0f + maxOffset, -1.0f, 1.0f - minOffset)
												// POSITIVE Z
												, glm::vec3(-1.0f + minOffset, 1.0f - maxOffset, 1.0f)
												, glm::vec3(-1.0f + minOffset, 1.0f - minOffset, 1.0f)												
												, glm::vec3(-1.0f + maxOffset, 1.0f - maxOffset, 1.0f)
												, glm::vec3(-1.0f + maxOffset, 1.0f - minOffset, 1.0f)
												// NEGATIVE Z
												, glm::vec3(1.0f - minOffset, 1.0f - maxOffset, -1.0f)
												, glm::vec3(1.0f - minOffset, 1.0f - minOffset, -1.0f)												
												, glm::vec3(1.0f - maxOffset, 1.0f - maxOffset, -1.0f)
												, glm::vec3(1.0f - maxOffset, 1.0f - minOffset, -1.0f)
	};

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(2, VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeMapTexCoords), cubeMapTexCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0,0);
	glEnableVertexAttribArray(1);

	firstTime = false;
	};
#endif

	glBindBuffer(GL_UNIFORM_BUFFER, g_Camera.UBO);	
	g_Camera.projectionMatrix = glm::perspectiveFov(90.f, (float)size, (float)size, 0.1f, 1000.f);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(g_Camera.projectionMatrix));
	for (auto i = 0; i < 6; ++i) 
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, g_CubeMapDynamicTextureID, 0);	
#ifndef CLEAR_CUBEMAP_FACE_WITH_SKYBOX
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_LEQUAL);
		glUseProgram(program);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLE_STRIP, i*4, 4);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);
#endif		
		g_Camera.viewMatrix = glm::lookAt(glm::vec3(0.f), cubeMapDir[i], cubeMapUp[i]);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(g_Camera.viewMatrix));
		RenderCubes();
		
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	error = glGetError(); assert(error == GL_NO_ERROR);
}

void Render()
{
	/*glBindFramebuffer(GL_FRAMEBUFFER, g_CubeMapFBO);
	glViewport(0, 0, 1024, 1024);
	for (auto i = 0; i < 6; ++i) 
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, g_CubeMapDynamicTextureID, 0);	
		glClear(GL_COLOR_BUFFER_BIT);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);*/
		
	RenderToCubeMap();

	auto width = glutGet(GLUT_WINDOW_WIDTH);
	auto height = glutGet(GLUT_WINDOW_HEIGHT);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	glViewport(0, 0, width, height);
	
	g_Camera.projectionMatrix = glm::perspectiveFov(45.f, (float)width, (float)height, 0.1f, 1000.f);
	
	// variables uniformes (constantes) 
	
	// rotation orbitale de la camera
	float rotY = glm::radians(g_Camera.rotation.y);
	const glm::vec4 orbitDistance(0.0f, 0.0f, 5.0f, 1.0f);
	glm::vec4 position = glm::eulerAngleY(rotY) * orbitDistance;
	g_Camera.viewMatrix = glm::lookAt(glm::vec3(position), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));

	glBindBuffer(GL_UNIFORM_BUFFER, g_Camera.UBO);
	//glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, glm::value_ptr(g_Camera.viewMatrix), GL_STREAM_DRAW);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4) * 2, glm::value_ptr(g_Camera.viewMatrix));

	float yaw = glm::radians(g_Cubes[0].rotation.y);
	float pitch = glm::radians(g_Cubes[0].rotation.x);
	float roll = glm::radians(g_Cubes[0].rotation.z);
	g_Cubes[0].worldMatrix = glm::eulerAngleYXZ(yaw, pitch, roll) * glm::mat4(0.1);

	//
	// rendu	
	//

	// cubes
	
	 RenderCubes();
	
	// sphere

	auto program = g_EnvMapShader.GetProgram();
	if (g_EnableLocalReflections) {
		program = g_LocalEnvMapShader.GetProgram();
	}
	glUseProgram(program);
	
	auto worldLocation = glGetUniformLocation(program, "u_worldMatrix");		

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, g_CubeMapDynamicTextureID);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, g_CubeMap.textureObj);

	glUniform3fv(glGetUniformLocation(program, "u_eyePosition"), 1, &position.x);
	//glm::mat4& transform = ;
	glUniformMatrix4fv(worldLocation, 1, GL_FALSE, glm::value_ptr(g_Sphere.worldMatrix));

	glBindVertexArray(g_Sphere.VAO);
	glDrawElements(GL_TRIANGLES, g_Sphere.ElementCount, GL_UNSIGNED_INT, 0);

	
	// dessin de la cubemap/skybox, de preference en dernier afin de limiter "l'overdraw"
		
	glUseProgram(g_SkyboxShader.GetProgram());

	// ces appels sont redondants, car cubemap deja bind precedemment
	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, g_CubeMap.textureObj);
	
	// Tres important ! 
	// D'une part, comme la cubemap represente un environnement distant (sky box)
	// il n'est pas utile d'ecrire dans le depth buffer (on est toujours au plus loin)
	// Cependant il faut quand meme effectuer le test de profondeur 
	// d'ou le fait que l'on n'a pas fait d'appel a glDisable(GL_DEPTH_TEST).
	// Neamoins il faut changer l'operateur du test dans le cas ou il y'aurai deja 
	// des valeurs egales a 1.0 dans le depth buffer (la skybox est l'element le plus eloignee)	
    glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
	
	glBindVertexArray(g_CubeMap.VAO);
	glDrawArrays(GL_TRIANGLES, 0, 8 * 2 * 3);
	
	// on retabli ensuite les render states par defaut
    glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);

	//
	// post rendu
	//

	glBindVertexArray(0);

	//glBindTexture(GL_TEXTURE_2D, 0);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	// dessine les tweakBar
	TwDraw();  

	glutSwapBuffers();
}

int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	glutCreateWindow("OBJ Loader & Environment Mapping");

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

