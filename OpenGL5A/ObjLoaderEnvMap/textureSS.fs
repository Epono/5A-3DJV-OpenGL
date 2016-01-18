 #version 150

in vec3 v_texCoords;

uniform samplerCube u_sampler;

//uniform vec3 u_texCoords;

out vec4 Fragment;

void main(void)
{
    Fragment = texture(u_sampler, v_texCoords);
}