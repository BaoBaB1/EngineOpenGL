#version 440 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aColor;
layout (location = 3) in vec2 aTextCoord;

layout (std140, binding = 0) uniform CameraData
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
} camData;

uniform mat4 modelMatrix;
uniform mat4 lightSpaceVPMatrix;

out vec3 normal;
out vec4 color;
out vec3 fragment;
out vec2 uv;
out vec4 fragPosLightSpace;

void main()
{
	gl_Position = (camData.projectionMatrix * camData.viewMatrix * modelMatrix) * vec4(aPos, 1.0);
	fragment = vec3(modelMatrix * vec4(aPos, 1.0f));
	normal = normalize(transpose(inverse(mat3(modelMatrix))) * aNormal);
	color = aColor;
	uv = aTextCoord;
	fragPosLightSpace = lightSpaceVPMatrix * vec4(fragment, 1);
}