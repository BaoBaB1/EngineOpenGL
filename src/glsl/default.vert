#version 440 core

const int g_directionalLightType = 0;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aColor;
layout (location = 3) in vec2 aTextCoord;

struct LightInfo
{
	// common info
	vec4 pos;
	vec4 dir;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	mat4 shadowMatrix;

	// attenuation info for point light
	float constant;
	float linear;
	float quadratic;

	// info for spot light
	float cutoff;
	float outerCutoff;
	int smoothSpotLight;

	// 0 - directional, 1 - point, 2 - spot
	int type;
};

layout (std140, binding = 0) uniform CameraData
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
} camData;

layout (std430, binding = 2) buffer Lights
{
	LightInfo lightInfos[];
};

uniform mat4 modelMatrix;
uniform int numLights;

out vec3 normal;
out vec4 color;
out vec3 fragment;
out vec2 uv;
out vec4 fragPosDirectionalLightSpace;

void main()
{
	gl_Position = (camData.projectionMatrix * camData.viewMatrix * modelMatrix) * vec4(aPos, 1.0);
	fragment = vec3(modelMatrix * vec4(aPos, 1.0f));
	normal = normalize(transpose(inverse(mat3(modelMatrix))) * aNormal);
	color = aColor;
	uv = aTextCoord;
	for (int i = 0; i < numLights; i++)
	{
		if (lightInfos[i].type == g_directionalLightType)
		{
			fragPosDirectionalLightSpace = lightInfos[i].shadowMatrix * vec4(fragment, 1);
		}
	}
}