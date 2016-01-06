 #version 150

 uniform sampler2D u_sampler;

in Vertex
{
    vec3 normal;
    vec2 texcoords;
    vec3 positionVS;    // VS = View Space
} IN;

layout(location = 0) out vec4 fragmentPosition;
layout(location = 1) out vec4 fragmentNormal;
layout(location = 2) out vec4 fragmentColor;

void main(void)
{    
    FragmentPosition = vec4(IN.positionVS, 1.0);
    FragmentNormal = vec4(IN.normal, 0.0);
    FragmentColor = texture(u_sampler, IN.texCoords;
}