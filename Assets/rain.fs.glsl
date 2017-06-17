#version 410

in vec3 vv3color;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(vv3color, 1.0);
}