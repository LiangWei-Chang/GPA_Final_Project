#version 410 core

uniform samplerCube tex_cubemap;
uniform int fogOn;

in VS_OUT {
    vec3 tc;
} fs_in;

layout (location = 0) out vec4 color;

void main(void)
{
	vec4 skyColor = vec4(0.5, 0.5, 0.5, 1.0);
	if(fogOn == 1){
		color = skyColor;
	}else{
		color = texture(tex_cubemap, fs_in.tc);
	}
    
}
