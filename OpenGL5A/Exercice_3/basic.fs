#version 330

uniform sampler2D u_texture;
in vec3 v_texCoords;

out vec4 Fragment;

void main(void)
{
    //Fragment = vec4(1.0, 1.0, 1.0, 0.0);
    Fragment = texture(u_texture, v_texCoords.xy);
}