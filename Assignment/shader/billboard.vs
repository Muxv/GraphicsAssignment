#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 CameraRight;
uniform vec3 CameraUp;
uniform vec3 CenterPos;

void main()
{
	vec3 OldPosition = vec3(model * vec4(aPos, 1.0f));
	vec3 NewPostion = OldPosition + CameraRight * aPos.x * 0.5 + CameraUp * aPos.y * 0.3;
	//gl_Position = projection * view * model * vec4(aPos, 1.0);
	gl_Position = projection * view * vec4(NewPostion, 1.0);
	TexCoords = aTexCoords;	
}