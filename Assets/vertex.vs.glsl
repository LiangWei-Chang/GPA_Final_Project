#version 410

layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;

uniform mat4 um4mv;
uniform mat4 um4p;

const float density = 0.1f;
const float gradient = 1.25f;

out VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 H; // eye space halfway vector
    vec2 texcoord;
} vertexData;

out float visibility;

void main()
{
	vec4 P = um4mv * vec4(iv3vertex, 1.0);
	gl_Position = um4p * P;
    vertexData.texcoord = iv2tex_coord;

	float distance = length(P.xyz);
	visibility = exp(-pow(((distance + 10) * density), gradient));
	visibility = clamp(visibility, 0.0f, 1.0f);
	visibility = 1.0f - visibility;

	vec3 V = normalize(-P.xyz);
    //[TODO] V represents the vector from eye to the vertex (in eye space) (should be normalized)
    vertexData.N = normalize(mat3(um4mv) * iv3normal);
    //[TODO] eye space normal (should be normalized)
    vertexData.L = normalize(vec3(1.0,1.0,1.0));
    //[TODO] eye space light vector(light direction) (should be normalized)
    vertexData.H = normalize(vertexData.L+V);
    //[TODO] eye space halfway vector (should be normalized)
}
