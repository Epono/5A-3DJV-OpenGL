
#include "Common.h"
#include "ParticleSystem.h"

uint32_t query;

ParticleSystem::ParticleSystem(ParticleBackEnd backend) : m_BackEnd(backend) {
	memset(m_GPUStorage.VBO, 0, sizeof(GLuint) * 2);
	memset(m_GPUStorage.VAO, 0, sizeof(GLuint) * 2);
}

ParticleSystem::~ParticleSystem() {
}

void ParticleSystem::Destroy() {
	glDeleteBuffers(2, m_GPUStorage.VBO);
	glDeleteVertexArrays(2, m_GPUStorage.VAO);

	m_Particles.clear();
}


void ParticleSystem::Emit(uint32_t count) {
	m_Count = count;

	if(m_GPUStorage.VAO[0] == 0) {
		GLsizei stride = sizeof(glm::vec3);

		glEnable(GL_POINT_SPRITE);
		glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
		glPointSize(8.f);

		m_ParticleShader.LoadVertexShader("particle.vs");
		m_ParticleShader.LoadFragmentShader("particle.fs");
		m_ParticleShader.Create();

		auto program = m_ParticleShader.GetProgram();
		auto blockIndex = glGetUniformBlockIndex(program, "ViewProj");
		glUniformBlockBinding(program, blockIndex, 0);

		glGenBuffers(2, m_GPUStorage.VBO);
		glGenVertexArrays(2, m_GPUStorage.VAO);

		if(m_BackEnd == ParticleBackEnd::CPU) {
			glBindVertexArray(m_GPUStorage.VAO[0]);
			glBindBuffer(GL_ARRAY_BUFFER, m_GPUStorage.VBO[0]);
			glBufferData(GL_ARRAY_BUFFER, count * stride, nullptr, GL_DYNAMIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
			glEnableVertexAttribArray(0);
		}
		else if(m_BackEnd == ParticleBackEnd::TransformFeedback) {
			stride = 2 * sizeof(glm::vec3);
			for(auto index = 0; index < 2; ++index) {
				glBindVertexArray(m_GPUStorage.VAO[index]);
				glBindBuffer(GL_ARRAY_BUFFER, m_GPUStorage.VBO[index]);
				glBufferData(GL_ARRAY_BUFFER, count * stride, nullptr, GL_DYNAMIC_DRAW);
				// position
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
				glEnableVertexAttribArray(0);
				// velocity
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*) (stride / 2));
				glEnableVertexAttribArray(0);
			}
		}

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	if(m_BackEnd == ParticleBackEnd::CPU) {
		for(auto index = 0; index < count; ++index) {
			Particle particle;

			float rndx = (rand() / (float) RAND_MAX)*30.f - 15.f;
			float rndy = (rand() / (float) RAND_MAX)*5.f - 2.5f;
			float rndz = (rand() / (float) RAND_MAX)*5.f - 2.5f;
			particle.position = glm::vec3(rndx, rndy, rndz);
			particle.velocity = glm::vec3(0.0);
			particle.color.a = 1.0f;

			m_Particles.push_back(particle);
		}
	}
	else {
		m_CurrentSource = 0;
		glBindBuffer(GL_ARRAY_BUFFER, m_GPUStorage.VBO[0]);
		glm::vec3* particles = (glm::vec3*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		if(m_BackEnd == ParticleBackEnd::TransformFeedback) {
			for(auto index = 0; index < count; ++index) {
				Particle particle;
				float rndx = (rand() / (float) RAND_MAX)*30.f - 15.f;
				float rndy = (rand() / (float) RAND_MAX)*5.f - 2.5f;
				float rndz = (rand() / (float) RAND_MAX)*5.f - 2.5f;
				*particles++ = glm::vec3(rndx, rndy, rndz);
				*particles++ = glm::vec3(0.0);
			}
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
}

float distance(glm::vec3 particlePos, glm::vec3 obstaclePos) {
	return (obstaclePos.x - particlePos.x) * (obstaclePos.x - particlePos.x) + (obstaclePos.y - particlePos.y) * (obstaclePos.y - particlePos.y);
}

void ParticleSystem::Update(float deltaTime) {
	if(m_BackEnd == ParticleBackEnd::CPU) {
		const glm::vec3 acceleration = glm::vec3(0.f, -9.81f, 0.f) * deltaTime; 	//  a*dt
		const glm::vec3 acceleration2 = acceleration * deltaTime * 0.5f; 		//  a*dt^2*0.5f

		for(auto index = 0; index < m_Particles.size(); ++index) {
			Particle &particle = m_Particles[index];

			particle.position += particle.velocity * deltaTime + acceleration2;
			particle.velocity += acceleration;

			for(auto indexo = 0; indexo < m_Obstacles.size(); ++indexo) {
				Obstacle &obstacle = m_Obstacles[indexo];

				if(distance(particle.position, obstacle.position) < obstacle.radius * obstacle.radius)
					particle.velocity.y = -particle.velocity.y;
			}
		}
	}
	else if(m_BackEnd == ParticleBackEnd::TransformFeedback) {
		Transform();
	}
	else {
		Compute();
	}
}

void ParticleSystem::Render() {
	// note: Pas besoin de matrice world, les positions sont deja exprimees dans le repere du monde

	m_ParticleShader.Bind();
	GLenum error = glGetError();
	if(m_BackEnd == ParticleBackEnd::CPU) {
		glBindBuffer(GL_ARRAY_BUFFER, m_GPUStorage.VBO[0]);
		glm::vec3 *vertices = (glm::vec3 *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

		for(auto index = 0; index < m_Particles.size(); ++index) {
			vertices[index] = m_Particles[index].position;
		}

		glUnmapBuffer(GL_ARRAY_BUFFER);

		glBindVertexArray(m_GPUStorage.VAO[0]);
		glDrawArrays(GL_POINTS, 0, m_Count);
	}
	else {
		// on affiche le vbo non transforme
		glBindVertexArray(m_GPUStorage.VAO[m_CurrentSource]);
		// on swap les buffers
		m_CurrentSource ^= 1;
		glDrawArrays(GL_POINTS, 0, m_Count);
		error = glGetError();
	}
}

void ParticleSystem::Transform() {
}

void ParticleSystem::Compute() {
}
