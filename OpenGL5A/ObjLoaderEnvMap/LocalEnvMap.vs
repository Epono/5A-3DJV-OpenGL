#version 150

#extension GL_ARB_explicit_attrib_location : enable

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texcoords;

uniform mat4 u_worldMatrix;

//const vec3 u_eyePosition = vec3(0.0, 0.0, 10.0);
uniform vec3 u_eyePosition;

layout(std140) uniform ViewProj
{
	mat4 u_viewMatrix;
	mat4 u_projectionMatrix;
};

out Vertex
{
	vec3 normal;
	vec2 texcoords;
	float fresnelTerm;
	vec3 reflectedDir;
	vec3 refractedDir;
	vec3 positionWS;
} OUT;

void main(void)
{	
	vec4 position = u_worldMatrix * vec4(a_position, 1.0);
	vec3 N = mat3(u_worldMatrix) * a_normal;

	//OUT.eyeDirection = normalize(eyePos - position.xyz);
	//OUT.eyeDirection = eyePos;
	OUT.positionWS = position.xyz;	
	OUT.normal = N;
	OUT.texcoords = a_texcoords;

	vec3 V = normalize(u_eyePosition - position.xyz);
	vec3 incident = -V;

	// Approximation de Christophe Schlick
    float FresnelCoeff = pow(clamp(dot(N, V), 0.0, 1.0), 4.0);
	OUT.fresnelTerm = FresnelCoeff;

	OUT.reflectedDir = reflect(incident, N);
	OUT.refractedDir = refract(incident, N, 1.1);
	
	gl_Position = u_projectionMatrix * u_viewMatrix * position;
}