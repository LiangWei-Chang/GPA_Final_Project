#version 410

layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;

uniform mat4 um4mv;
uniform mat4 um4p;

const float density = 0.1f;
const float gradient = 1.5f;

out VertexData
{
    vec3 eyeDir;
    vec3 lightDir;
    vec3 normal;
    vec2 texcoord;
} vertexData;

out float visibility;

void main()
{
	vec4 position2Camera = um4mv * vec4(iv3vertex, 1.0); 
	gl_Position = um4p * position2Camera;
    vertexData.texcoord = iv2tex_coord;

	float distance = length(position2Camera.xyz);
	visibility = exp(-pow((distance * density), gradient));
	visibility = clamp(visibility, 0.0f, 1.0f);
	visibility = 1.0f - visibility;
}
