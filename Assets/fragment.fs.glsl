#version 410

layout(location = 0) out vec4 fragColor;

uniform mat4 um4mv;
uniform mat4 um4p;
uniform int needDiscard;

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
    vec4 texColor = texture(tex_color, vertexData.texcoord);
    if(texColor.a < 0.5)
    	discard;
    else
    	fragColor = texColor;
}
