#version 150

#extension GL_ARB_explicit_attrib_location : enable

layout(location = 0) in vec4 a_position;

uniform mat4 u_worldMatrix;

layout(std140) uniform ViewProj
{
	mat4 u_viewMatrix;
	mat4 u_projectionMatrix;
};

void main(void)
{
	gl_Position = u_projectionMatrix * u_viewMatrix * u_worldMatrix * a_position;
}