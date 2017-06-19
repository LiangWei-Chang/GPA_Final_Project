#version 410

layout(location = 0) out vec4 fragColor;

uniform mat4 um4mv;
uniform mat4 um4p;

in VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 H; // eye space halfway vector
    vec2 texcoord;
} vertexData;

in float visibility;

uniform sampler2D tex_color;
uniform int fogOn;

void main()
{
	float theta = max(dot(vertexData.N, vertexData.L),0.0);
    //[TODO] theta represents the angle between N and L, and theta should always >= 0.0
    float phi = max(dot(vertexData.N, vertexData.H),0.0);
    //[TODO] phi represents the angle between N and H, and phi should always >= 0.0
    vec3 texColor = texture(tex_color, vertexData.texcoord).rgb;
    
    vec3 ambient = texColor * vec3(0.3,0.3,0.3);
    //[TODO] ambient = Ka*Ia
    vec3 diffuse = texColor * vec3(0.5,0.5,0.5) * theta;
    //[TODO] diffuse = Kd*Id*theta
    vec3 specular = vec3(1,1,1) * vec3(1,1,1) * pow(phi,100);
    //[TODO] specular = Ks*Is*pow(phi,shinness)
    vec4 frag = vec4(ambient + diffuse + specular, 1.0);

    vec4 temp = texture(tex_color, vertexData.texcoord);
	vec4 skyColor = vec4(0.5, 0.5, 0.5, 1.0);
	
    if(temp.a < 0.5)
    	discard;
    else{
		fragColor = frag;
		if(fogOn == 1){
			fragColor = mix(fragColor, skyColor, visibility);
		}
	}
}
