#version 330

in vec4 v_color;

out vec4 Fragment;

void main(void)
{
    Fragment = v_color;
    //Fragment = vec4(1.0, 0.0, 0.0, 0.0);
}