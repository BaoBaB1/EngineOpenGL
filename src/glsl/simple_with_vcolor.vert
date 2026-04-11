#version 440 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 color;

layout (std140, binding = 0) uniform CameraData
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
} camData;

uniform mat4 modelMatrix;
out vec4 vColor;

void main()
{
    gl_Position = (camData.projectionMatrix * camData.viewMatrix * modelMatrix) * vec4(aPos, 1.0);
		vColor = color;
}
