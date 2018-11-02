#version 330 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;

void main() 
{
    vec4 Color = texture(texture_diffuse1, TexCoords);
	if(Color.a == 0){
		discard;
	}

    FragColor = Color;//Color;
	//float brightness = dot(Color, vec3(0.2126, 0.7152, 0.0722));
    //if(brightness >= 1.0)
    //    BrightColor = vec4(Color, 1.0);
    //else
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}