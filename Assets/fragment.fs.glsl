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

in float visibility;

uniform sampler2D tex_color;
uniform int fogOn;

void main()
{
    vec4 texColor = texture(tex_color, vertexData.texcoord);
	vec4 skyColor = vec4(0.5, 0.5, 0.5, 1.0);
	//vec4 skyColor = vec4(1.0, 1.0, 1.0, 1.0);
    if(texColor.a < 0.5)
    	discard;
    else{
		fragColor = texColor;
		if(fogOn == 1){
			fragColor = mix(fragColor, skyColor, visibility);
		}
		
		
			
			
	}
    	
}
