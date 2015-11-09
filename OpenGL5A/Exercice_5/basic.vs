#version 330

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec3 a_normal;

uniform int u_hasTexcoords;

in vec2 a_texCoords;

uniform int u_hasNormals;

uniform mat4 u_worldMatrix;
layout(std140) uniform ViewProj
{
	mat4 u_viewMatrix;
	mat4 u_projectionMatrix;
};

out vec3 v_normal;
out vec2 v_texCoords;

void main(void)
{
	if(u_hasNormals != 0) {
		vec3 N = mat3(u_worldMatrix) * a_normal;
		v_normal = N;
	}

	if(u_hasTexcoords != 0) {
		v_texCoords = a_texCoords;
	}

	gl_Position = u_projectionMatrix * u_viewMatrix * u_worldMatrix * a_position;
}