//
// ESGI
// Multiple Point Lights (forward et deferred rendering)
//


#include "Common.h"

#include <cstdio>
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>

#include "../common/EsgiShader.h"

#include "GBuffer.h"
#include "Scene.h"

#include "AntTweakBar.h"

// ---

TwBar* objTweakBar;
bool g_EnableMultipass;
bool g_EnablePrePass;
bool g_EnableDeferred;
bool g_EnablePositionTexture = 1;

uint32_t g_NumPointLights = 1;

enum RENDERMODE 
{
	SINGLEPASS = 0,
	MULTIPASS,
	DEFERRED_WITH_POS,
	DEFERRED,
	RENDERMODE_MAX
};

RENDERMODE g_currentRenderMode = SINGLEPASS;

int previousTime = 0;

// ---

GBuffer g_GBuffer;

EsgiShader g_BlinnPhongSinglePassShader;
EsgiShader g_BlinnPhongMultiPassShader;
EsgiShader g_BlinnPhongDeferredFatShader;
EsgiShader g_BlinnPhongDeferredShader;
EsgiShader g_AmbientShader;
EsgiShader g_GBufferFatShader;
EsgiShader g_GBufferShader;

const char* shaders[] = { "blinnPhong_SinglePass.vs", "blinnPhong_SinglePass.fs"
						, "blinnPhong_MultiPass.vs", "blinnPhong_MultiPass.fs"
						, "blinnPhong_DeferredFat.vs", "blinnPhong_DeferredFat.fs"
						, "blinnPhong_Deferred.vs", "blinnPhong_Deferred.fs" 
						, nullptr, nullptr
						, "ambient.vs", "ambient.fs"
						, "gbufferFat.vs", "gbufferFat.fs"
						, "gbuffer.vs", "gbuffer.fs"
						, nullptr
					};

EsgiShader* shaderClasses[] = { &g_BlinnPhongSinglePassShader
							, &g_BlinnPhongMultiPassShader
							, &g_BlinnPhongDeferredFatShader
							, &g_BlinnPhongDeferredShader
							, nullptr 
							, &g_AmbientShader
							, &g_GBufferFatShader
							, &g_GBufferShader
							, nullptr
						};

// ---

GLuint PointLight::UBO;
GLuint Material::UBO;

extern Vertex g_Room[36];

Material g_ShinyMaterial = {
    glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
    glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
    glm::vec4(1.0f, 1.0f, 1.0f, 32.0f)
};


Walls g_Walls;
#define MAX_POINT_LIGHTS	128
PointLight g_PointLights[MAX_POINT_LIGHTS];

Mesh g_SphereMesh;
std::vector<Sphere> g_Spheres;

Mesh g_WallMesh;


struct ViewProj
{
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
	glm::mat4 inverseProjectionMatrix;

	glm::vec3 rotation;
	GLuint UBO;	
	bool autoRotateCamera;
} g_Camera;

// ---


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
	TwAddSeparator(objTweakBar, "options", "");
	TwAddVarRW(objTweakBar, "Active Multipass", TW_TYPE_BOOL8, &g_EnableMultipass, "");
	TwAddVarRW(objTweakBar, "Active Deferred", TW_TYPE_BOOL8, &g_EnableDeferred, "");
	TwAddVarRW(objTweakBar, "Fat G-Buffer", TW_TYPE_BOOL8, &g_EnablePositionTexture, "");
	//TwAddVarRW(objTweakBar, "Active Depth Pre-Pass", TW_TYPE_BOOL8, &g_EnablePrePass, "");
	TwAddSeparator(objTweakBar, "programme", "");
	TwAddButton(objTweakBar, "Quitter", &ExitCallbackTw, nullptr, "");
	
	// Objets OpenGL

	glGenBuffers(1, &g_Camera.UBO);
	glBindBuffer(GL_UNIFORM_BUFFER, g_Camera.UBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 3, nullptr, GL_STREAM_DRAW);
	

	glGenBuffers(1, &PointLight::UBO);
	glBindBuffer(GL_UNIFORM_BUFFER, PointLight::UBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(PointLight) * MAX_POINT_LIGHTS, nullptr, GL_STREAM_DRAW);
	

	glGenBuffers(1, &Material::UBO);
	glBindBuffer(GL_UNIFORM_BUFFER, Material::UBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(Material), &g_ShinyMaterial, GL_STATIC_DRAW);
	

	auto index = 0;
	EsgiShader* shaderClass = shaderClasses[index];
	while (shaderClass)
	{
		shaderClasses[index]->LoadVertexShader(shaders[index * 2]);
		shaderClasses[index]->LoadFragmentShader(shaders[index * 2 + 1]);
		shaderClasses[index]->Create();
		auto program = shaderClasses[index]->GetProgram();

		glBindBufferBase(GL_UNIFORM_BUFFER, 0, g_Camera.UBO); // deja bound
		auto blockIndex = glGetUniformBlockIndex(program, "ViewProj");
		glUniformBlockBinding(program, blockIndex, 0);

		glBindBufferBase(GL_UNIFORM_BUFFER, 1, PointLight::UBO);
		blockIndex = glGetUniformBlockIndex(program, "Lights");
		glUniformBlockBinding(program, blockIndex, 1);

		glBindBufferBase(GL_UNIFORM_BUFFER, 2, Material::UBO);
		blockIndex = glGetUniformBlockIndex(program, "Material");
		glUniformBlockBinding(program, blockIndex, 2);

		shaderClass = shaderClasses[++index];
	}

	shaderClass = shaderClasses[++index];
	while (shaderClass)
	{
		shaderClasses[index]->LoadVertexShader(shaders[index * 2]);
		shaderClasses[index]->LoadFragmentShader(shaders[index * 2 + 1]);
		shaderClasses[index]->Create();
		auto program = shaderClasses[index]->GetProgram();

		glBindBufferBase(GL_UNIFORM_BUFFER, 0, g_Camera.UBO);
		auto blockIndex = glGetUniformBlockIndex(program, "ViewProj");
		glUniformBlockBinding(program, blockIndex, 0);

		shaderClass = shaderClasses[++index];
	}

	// Setup scene

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
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);

	const std::string inputFile = "sphere.obj";
	LoadOBJ(g_SphereMesh, inputFile);

	g_Spheres.resize(g_NumPointLights);
	for (uint32_t index = 0; index < g_NumPointLights; ++index)
	{
		g_Spheres[index].initialize(g_PointLights[index]);
	}

	g_GBuffer.Create();
	g_GBuffer.Unbind();
}

void Terminate()
{		
	g_GBuffer.Destroy();

	TwTerminate();

	glDeleteBuffers(1, &Material::UBO);
	glDeleteBuffers(1, &PointLight::UBO);
	glDeleteBuffers(1, &g_Camera.UBO);

	g_Spheres.shrink_to_fit();
	
	CleanMesh(g_SphereMesh);

	glDeleteTextures(3, g_Walls.textures);
	CleanMesh(g_WallMesh);

	auto index = 0;
	EsgiShader* shaderClass = shaderClasses[index];
	while (shaderClass)
	{
		shaderClass->Destroy();
		shaderClass = shaderClasses[++index];
	}
	shaderClass = shaderClasses[++index];
	while (shaderClass)
	{
		shaderClass->Destroy();
		shaderClass = shaderClasses[++index];
	}
}

// boucle principale ---

void Resize(GLint width, GLint height) 
{
	glViewport(0, 0, width, height);
	
	g_Camera.projectionMatrix = glm::perspectiveFov(45.f, (float)width, (float)height, 0.1f, 1000.f);
	g_Camera.inverseProjectionMatrix = glm::inverse(g_Camera.projectionMatrix);

	g_GBuffer.width = width;
	g_GBuffer.height = height;

	TwWindowSize(width, height);
}

static bool g_CanDraw = false;

void Update() 
{
	if (g_EnableDeferred) {	
		g_EnableMultipass = true;
		g_currentRenderMode = (RENDERMODE)(DEFERRED - g_EnablePositionTexture);
	}
	else {
		g_currentRenderMode = (RENDERMODE)g_EnableMultipass;
	}	

	auto currentTime = glutGet(GLUT_ELAPSED_TIME);
	auto delta = currentTime - previousTime;
	previousTime = currentTime;
	auto elapsedTime = delta / 1000.0f;
	
	if (g_Camera.autoRotateCamera) {
		g_Camera.rotation.y += 10.f * elapsedTime;
	}

	if (g_Spheres.size() != g_NumPointLights) {
		if (g_Spheres.size() < g_NumPointLights)
		{
			for (auto index = g_Spheres.size(); index < g_NumPointLights; ++index) {
				g_Spheres.push_back(Sphere()); // pas terrible, cf cours C++ avance
				g_Spheres[index].initialize(g_PointLights[index]);
			}
		}
		else {
			g_Spheres.pop_back();
		}
	}

	for (uint32_t index = 0; index < g_NumPointLights; ++index)
	{
		g_Spheres[index].update(elapsedTime);
	}

	static char title[256];
	static float timer = 0.1f;
	timer += elapsedTime;
	if (timer >= 0.1f) {
		static const char* renderModeText[] = { "forward (single pass)", "forward (multi pass)", "deferred shading (fat)", "deferred shading" };
		sprintf(title, "%s [%s] (%3.2f ms approx. %4.2f fps)", "Multiple Point Lights", renderModeText[g_currentRenderMode], elapsedTime*1000.f, 1.0f / elapsedTime);
		glutSetWindowTitle(title);
		timer -= 0.1f;
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
		
	// ATTENTION ! L'ecriture dans le depth buffer doit etre active -glDepthMask(GL_TRUE) afin
	// que le depth buffer soit correctement efface ! 
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);

	glm::vec4 ambientColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearColor(ambientColor.r, ambientColor.g, ambientColor.b, ambientColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	

	// variables uniformes (constantes) 

	g_Camera.projectionMatrix = glm::perspectiveFov(45.f, (float)width, (float)height, 0.1f, 1000.f);
	// rotation orbitale de la camera
	float rotY = glm::radians(g_Camera.rotation.y);
	const glm::vec4 orbitDistance(0.0f, 0.0f, 200.0f, 1.0f);
	glm::vec4 position = /*glm::eulerAngleY(rotY) **/ orbitDistance;
	g_Camera.viewMatrix = glm::lookAt(glm::vec3(position), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
	// Mise a jour de toutes les matrices de l'UBO 
	glBindBuffer(GL_UNIFORM_BUFFER, g_Camera.UBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4) * 3, glm::value_ptr(g_Camera.viewMatrix));

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
	auto program = shaderClasses[SINGLEPASS]->GetProgram();
	if (g_EnableDeferred) 
	{		
		program = g_EnablePositionTexture ? g_GBufferFatShader.GetProgram() : g_GBufferShader.GetProgram();

	}
	else if (g_EnableMultipass) {
		program = shaderClasses[MULTIPASS]->GetProgram();
	}
	
	glUseProgram(program);	
	auto worldLocation = glGetUniformLocation(program, "u_worldMatrix");
	glm::mat4& transform = g_Walls.worldMatrix;
	glUniformMatrix4fv(worldLocation, 1, GL_FALSE, glm::value_ptr(transform));

	if (g_EnableDeferred) 
	{
		//
		// En deferred, le rendu s'effectue d'abord dans les G-Buffers
		//
		if (g_EnableDeferred) {
			g_GBuffer.Bind(g_EnablePositionTexture);
			// bien penser a effacer la depth texture en DEPTH_ATTACHMENT aussi 
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		
		auto startIndex = 6;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_Walls.textures[Walls::gWallTexture]);	
		glDrawArrays(GL_TRIANGLES, startIndex, 6 * 3); startIndex += 6 * 3;	// 4 murs
		glBindTexture(GL_TEXTURE_2D, g_Walls.textures[Walls::gCeilingTexture]);	
		glDrawArrays(GL_TRIANGLES, startIndex, 6); startIndex += 6;	// plafond
		glBindTexture(GL_TEXTURE_2D, g_Walls.textures[Walls::gFloorTexture]);	
		glDrawArrays(GL_TRIANGLES, startIndex, 6); startIndex += 6;	// sol

		g_GBuffer.Unbind();

		//
		// seconde passe : illumination
		//

		// ACTIVE LE BLENDING ADDITIF AFIN D'ACCUMULER LES LIGHTS
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		// En deferred shading on se contente de dessiner les light volumes le plus simplement

		// NE PAS INSCRIRE LES LIGHT VOLUMES DANS LE DEPTH BUFFER
		glDepthMask(GL_FALSE);

		glBindVertexArray(g_SphereMesh.VAO);
	
		program = shaderClasses[g_currentRenderMode]->GetProgram();
		glUseProgram(program);

		auto invScreenSizeLocation = glGetUniformLocation(program, "u_invScreenSize");
		glUniform2f(invScreenSizeLocation, 1.0f / (float)width, 1.0f / (float)height);

		worldLocation = glGetUniformLocation(program, "u_worldMatrix");	

		glActiveTexture(GL_TEXTURE0);
		if (g_EnablePositionTexture)
			glBindTexture(GL_TEXTURE_2D, g_GBuffer.positionTex);
		else
			glBindTexture(GL_TEXTURE_2D, g_GBuffer.depthTex);
		glUniform1i(glGetUniformLocation(program, "u_samplerPosition"), 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, g_GBuffer.normalTex);
		glUniform1i(glGetUniformLocation(program, "u_samplerNormal"), 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, g_GBuffer.albedoTex);
		glUniform1i(glGetUniformLocation(program, "u_samplerAlbedo"), 2);

		auto lightIndexLocation = glGetUniformLocation(program, "u_lightIndex");
		for (auto index = 0; index < g_Spheres.size(); ++index) 
		{
			glUniform1i(lightIndexLocation, index);	
			glm::mat4& transform = g_Spheres[index].worldMatrix;
			g_Spheres[index].setUniformScale(100.0f);
			glUniformMatrix4fv(worldLocation, 1, GL_FALSE, glm::value_ptr(transform));
			glDrawElements(GL_TRIANGLES, g_SphereMesh.ElementCount, GL_UNSIGNED_INT, 0);
		}

		glDisable(GL_BLEND);
	}
	else if (g_EnableMultipass)
	{
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_LEQUAL);
		// ACTIVE LE BLENDING ADDITIF AFIN D'ACCUMULER LES LIGHTS
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		auto lightIndexLocation = glGetUniformLocation(program, "u_lightIndex");	
		for (auto index = 0; index < g_NumPointLights; ++index) {
			glUniform1i(lightIndexLocation, index);

			auto startIndex = 6;
			glBindTexture(GL_TEXTURE_2D, g_Walls.textures[Walls::gWallTexture]);	
			glDrawArrays(GL_TRIANGLES, startIndex, 6 * 3); startIndex += 6 * 3;	// 4 murs
			glBindTexture(GL_TEXTURE_2D, g_Walls.textures[Walls::gCeilingTexture]);	
			glDrawArrays(GL_TRIANGLES, startIndex, 6); startIndex += 6;	// plafond
			glBindTexture(GL_TEXTURE_2D, g_Walls.textures[Walls::gFloorTexture]);	
			glDrawArrays(GL_TRIANGLES, startIndex, 6); startIndex += 6;	// sol
		}

		glDepthFunc(GL_LESS);
		glDisable(GL_BLEND);	
	}
	else
	{
		auto numLightsLocation = glGetUniformLocation(program, "u_numLights");
		glUniform1i(numLightsLocation, g_NumPointLights);

		auto startIndex = 6;
		glBindTexture(GL_TEXTURE_2D, g_Walls.textures[Walls::gWallTexture]);	
		glDrawArrays(GL_TRIANGLES, startIndex, 6 * 3); startIndex += 6 * 3;	// 4 murs
		glBindTexture(GL_TEXTURE_2D, g_Walls.textures[Walls::gCeilingTexture]);	
		glDrawArrays(GL_TRIANGLES, startIndex, 6); startIndex += 6;	// plafond
		glBindTexture(GL_TEXTURE_2D, g_Walls.textures[Walls::gFloorTexture]);	
		glDrawArrays(GL_TRIANGLES, startIndex, 6); startIndex += 6;	// sol
	}

	// rendu debug

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	program = g_AmbientShader.GetProgram();
	glUseProgram(program);

	worldLocation = glGetUniformLocation(program, "u_worldMatrix");

	glBindVertexArray(g_SphereMesh.VAO);
	for (auto index = 0; index < g_Spheres.size(); ++index) 
	{
		g_Spheres[index].setUniformScale(1.0f);
		glm::mat4& transform = g_Spheres[index].worldMatrix;
		glUniformMatrix4fv(worldLocation, 1, GL_FALSE, glm::value_ptr(transform));
		glDrawElements(GL_TRIANGLES, g_SphereMesh.ElementCount, GL_UNSIGNED_INT, 0);
	}

	glBindVertexArray(0);

	//g_GBuffer.DrawDebug(g_EnablePositionTexture);
	
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

