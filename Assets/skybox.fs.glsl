#version 410 core

uniform samplerCube tex_cubemap;
uniform int fogOn;

in VS_OUT {
    vec3 tc;
} fs_in;

const float upperLimit = 0.5f;
const float lowerLimit = -25.0f;

layout (location = 0) out vec4 color;

void main(void)
{
	vec4 skyColor = vec4(0.5, 0.5, 0.5, 1.0);
	vec4 finalColor = texture(tex_cubemap, fs_in.tc);
	float factor = (fs_in.tc.y - lowerLimit) / (upperLimit - lowerLimit);
	factor = clamp(factor, 0.0, 1.0);
	if(fogOn == 1){
		color = mix(skyColor, finalColor, 1 - factor);
	}else{
		color = finalColor;
	}
    
}
