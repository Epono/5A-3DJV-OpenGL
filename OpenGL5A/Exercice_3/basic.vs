#version 330

layout(location=0) in vec4 a_position;

uniform mat4 u_worldMatrix;

layout(std140) uniform ViewProj {
	mat4 u_viewMatrix; 
	mat4 u_projectionMatrix; 
};

out vec3 v_texCoords;

void main(void)
{
	v_texCoords = vec3(a_position.x + 0.5, -a_position.y + 0.5, a_position.z + 0.5);
	gl_Position = u_projectionMatrix * u_viewMatrix * u_worldMatrix * a_position;
}