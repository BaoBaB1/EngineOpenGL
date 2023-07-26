#version 440 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aColor;
layout (location = 3) in vec2 aTextCoord;

//uniform mat4 MVP;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec3 normal;
out vec4 color;
out vec3 fragment;
out vec2 textCoord;

void main()
{
	gl_Position = (projectionMatrix * viewMatrix * modelMatrix) * vec4(aPos, 1.0);
	fragment = vec3(modelMatrix * vec4(aPos, 1.0f));
	normal = transpose(inverse(mat3(modelMatrix))) * aNormal;
	color = aColor;
	textCoord = aTextCoord;
}