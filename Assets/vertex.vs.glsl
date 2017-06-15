#version 410

layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;

uniform mat4 um4mv;
uniform mat4 um4p;

out VertexData
{
    vec3 eyeDir;
    vec3 lightDir;
    vec3 normal;
    vec2 texcoord;
} vertexData;

void main()
{
	gl_Position = um4p * um4mv * vec4(iv3vertex, 1.0);
    vertexData.texcoord = iv2tex_coord;
}
