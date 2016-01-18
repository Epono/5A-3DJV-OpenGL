 #version 150

 // todo : uniform
const vec4 ambientColor = vec4(1.0, 1.0, 1.0, 1.0);
const float ambientIntensity = 1.0;

out vec4 Fragment;

void main(void)
{
    Fragment = ambientIntensity * ambientColor;
}