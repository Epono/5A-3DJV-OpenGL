#version 150

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_worldMatrix;
layout(std140) uniform ViewProj
{
	mat4 u_viewMatrix;
	mat4 u_projectionMatrix;
};

out vec3 v_normal;

void main(void)
{
	vec3 N = mat3(u_worldMatrix) * a_normal;
	v_normal = N;
	gl_Position = u_projectionMatrix * u_viewMatrix * u_worldMatrix * a_position;

}