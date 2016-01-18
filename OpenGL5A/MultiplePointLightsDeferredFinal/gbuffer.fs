 #version 150

#extension GL_ARB_explicit_attrib_location : enable

uniform sampler2D u_sampler;

in Vertex
{
	vec3 normal;
	vec2 texcoords;
} IN;

layout(location = 0) out vec4 FragmentNormal;
layout(location = 1) out vec4 FragmentColor;

void main(void)
{
    FragmentNormal = vec4(IN.normal, 0.0);
    FragmentColor = texture(u_sampler, IN.texcoords);
}
