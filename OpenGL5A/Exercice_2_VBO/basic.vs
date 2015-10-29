#version 330

layout(location=0) in vec4 a_position;

uniform mat4 u_worldMatrix;
uniform	mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;

uniform vec3 u_offset;
uniform vec4 u_color;

out vec4 v_color;

void main(void)
{
	gl_Position = u_projectionMatrix * u_viewMatrix * u_worldMatrix * (a_position + vec4(u_offset, 0.0f));
	v_color = u_color;
}