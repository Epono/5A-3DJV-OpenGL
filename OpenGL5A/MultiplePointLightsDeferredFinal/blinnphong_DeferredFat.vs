#version 150

#extension GL_ARB_explicit_attrib_location : enable

layout(location = 0) in vec4 a_position;

// note: a partir d'OpenGL 4.3 ou si l'extension GL_ARB_explicit_uniform_location 
//		est disponible, on peut egalement affecter un location aux variable uniformes

uniform mat4 u_worldMatrix;

// note: a partir d'OpenGL 4.2 ou si l'extension GL_ARB_shading_language_420pack
//		est disponible, on peut affecter un binding point aux UBOs avec layout(binding=?)

layout(std140) uniform ViewProj
{
	mat4 u_viewMatrix;
	mat4 u_projectionMatrix;
	mat4 u_inverseProjectionMatrix;
};

out vec3 viewDirVS;

void main(void)
{	
	vec4 viewPosition = u_viewMatrix * u_worldMatrix * a_position;
	gl_Position = u_projectionMatrix * viewPosition;
	viewDirVS = normalize(-viewPosition.xyz);
}