#version 440 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in mat4 instanceMatrix;

layout (std140, binding = 0) uniform CameraData
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
} camData;

void main()
{
  gl_Position = (camData.projectionMatrix * camData.viewMatrix * instanceMatrix) * vec4(aPos, 1.0);
}
