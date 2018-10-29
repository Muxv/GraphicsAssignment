#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

vec4 result;

void main()
{
    TexCoords = aPos;
	result = projection * view  * vec4(aPos, 1.0);
    gl_Position = result.xyww;

}