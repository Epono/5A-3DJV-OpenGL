#version 330

layout(location = 0) in vec4 a_position;

layout(std140) uniform ViewProj
{
	mat4 u_viewMatrix;
	mat4 u_projectionMatrix;
};

void main(void)
{
	gl_Position = u_projectionMatrix * u_viewMatrix * a_position;	
}