#version 440 core

layout (location = 0) in vec3 aPos;

//layout (std140, binding = 0) uniform CameraData
//{
//	mat4 viewMatrix;
//	mat4 projectionMatrix;
//} camData;

uniform mat4 lightViewProjMatrix;
uniform mat4 modelMatrix;

void main()
{
    gl_Position = lightViewProjMatrix * modelMatrix * vec4(aPos, 1.0);
}
