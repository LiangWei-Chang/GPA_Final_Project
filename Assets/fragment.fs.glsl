#version 410

layout(location = 0) out vec4 fragColor;

uniform mat4 um4mv;
uniform mat4 um4p;

in VertexData
{
    vec3 eyeDir;
    vec3 lightDir;
    vec3 normal;
    vec2 texcoord;
} vertexData;

uniform sampler2D tex_color;

void main()
{
    vec3 texColor = texture(tex_color, vertexData.texcoord);
    fragColor = vec4(texColor, 1.0);
}
