
#pragma once

#include <vector>

struct Particle
{
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec4 color;
};

struct Obstacle
{
	glm::vec3 position;
	float radius;
};

enum ParticleBackEnd
{
	CPU = 0,
	TransformFeedback,
	Compute
};

class ParticleSystem
{
public:
	ParticleSystem(ParticleBackEnd backend = CPU);
	~ParticleSystem();

	EsgiShader m_ParticleShader;
	EsgiShader m_TransformShader;
	EsgiShader m_ComputeShader;

	glm::vec3 m_Position;
	glm::vec3 m_Orientation;
	uint32_t m_Count;
	ParticleBackEnd m_BackEnd;
	uint8_t m_CurrentSource;

	struct GPUParticleSystem
	{
		GLuint VBO[2];
		GLuint VAO[2];
	} m_GPUStorage;

	std::vector<Particle> m_Particles;
	std::vector<Obstacle> m_Obstacles;

	void Emit(uint32_t count);

	void Destroy();

	void Update(float deltaTime);

	void Render();

	void Transform();

	void Compute();
};