#version 150

#extension GL_ARB_explicit_attrib_location : enable

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec2 a_texcoords;
layout(location = 2) in vec3 a_normal;

uniform mat4 u_worldMatrix;

layout(std140) uniform ViewProj
{
	mat4 u_viewMatrix;
	mat4 u_projectionMatrix;
	mat4 u_inverseProjectionMatrix;
};

out Vertex
{
	vec3 normal;
	vec2 texcoords;
	vec3 positionVS;	// VS = View Space
} OUT;

void main(void)
{
	mat4 worldViewMatrix = u_viewMatrix * u_worldMatrix;
	vec4 positionVS = worldViewMatrix * a_position;
	vec3 N = mat3(worldViewMatrix) * a_normal;
	OUT.normal = N;
	OUT.texcoords = a_texcoords;

	OUT.positionVS = positionVS.xyz;

	gl_Position = u_projectionMatrix * positionVS;
}