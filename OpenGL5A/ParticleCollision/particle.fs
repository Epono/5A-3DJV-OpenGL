 #version 330

uniform sampler2D u_sampler;

out vec4 Fragment;

void main(void)
{
    Fragment = vec4(0.0, 0.0, 1.0, 1.0);
}