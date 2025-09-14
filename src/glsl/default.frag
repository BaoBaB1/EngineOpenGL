#version 440 core

const int g_directionalLightType = 0;
const int g_pointLightType = 1;
const int g_spotLightType = 2;

struct Material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
	float alpha;
};

struct LightInfo
{
	// common info
	vec4 pos;
	vec4 dir;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;

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
	int shadowMatrixBufferIndex;
};

layout (std430, binding = 2) buffer Lights
{
	LightInfo lightInfos[];
};

out vec4 fragColor;

in vec3 normal;
in vec4 color;
in vec3 fragment;
in vec2 uv;
in vec4 fragPosDirectionalLightSpace;

uniform bool applyShading;
uniform bool hasDefaultTexture;
uniform bool hasAmbientTex;
uniform bool hasDiffuseTex;
uniform bool hasSpecularTex;
uniform vec3 viewPos;
uniform sampler2D defaultTexture;
uniform sampler2D ambientTex;
uniform sampler2D diffuseTex;
uniform sampler2D specularTex;
uniform sampler2D shadowMap;
uniform Material material;
uniform int numLights;

//vec2 poissonDisk[4] = vec2[](
//  vec2( -0.94201624, -0.39906216 ),
//  vec2( 0.94558609, -0.76890725 ),
//  vec2( -0.094184101, -0.92938870 ),
//  vec2( 0.34495938, 0.29387760 )
//);
//
//// Returns a random number based on a vec3 and an int.
//float random(vec3 seed, int i){
//	vec4 seed4 = vec4(seed,i);
//	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
//	return fract(sin(dot_product) * 43758.5453);
//}


float CalculateShadowValue(vec3 directionalLightDir)
{
	// perform perspective divide and scale coord to [-1, 1] range
	vec3 ndc = fragPosDirectionalLightSpace.xyz / fragPosDirectionalLightSpace.w;
	// scale to range [0, 1] because depth values in shadow map are in range [0, 1]
	ndc = ndc * 0.5 + 0.5;
	// everything that is out of shadow map or if normal is facing away from light
	float dp = dot(normal, -normalize(directionalLightDir));
	if (ndc.z > 1.f || dp <= 0.0f || dp < 0.1)
	{
		return 0;
	}
	// closest depth from light's perspective
	float closestDepth = texture(shadowMap, ndc.xy).r;
	float currentDepth = ndc.z;
	float bias = max(0.005 * (1.0 - dp), 0.001);
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	int n = 1;
	int total_texels = 0;
	for(int x = -n; x <= n; ++x)
	{
			for(int y = -n; y <= n; ++y)
			{
				total_texels++;
				float pcfDepth = texture(shadowMap, ndc.xy + vec2(x, y) * texelSize).r;
				shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
			}
	}
	return shadow / total_texels;
}

void main()
{
	fragColor = color;
	vec3 ambientColor = vec3(1);
	vec3 diffuseColor = vec3(1);
	vec3 specularColor = vec3(1);
	if (hasAmbientTex)
	{
		ambientColor = texture(ambientTex, uv).rgb;
	}
	if (hasDiffuseTex)
	{
		diffuseColor = texture(diffuseTex, uv).rgb;
		fragColor = vec4(diffuseColor, 1);
	}
	if (hasSpecularTex)
	{
		specularColor = texture(specularTex, uv).rgb;
	}

	if (applyShading && numLights > 0)
	{
		vec3 ambientMaterialComponent = material.ambient;
		vec3 diffuseMaterialComponent = material.diffuse;
		vec3 specularMaterialComponent = material.specular;
		float shininess = material.shininess;
		float alpha = material.alpha;

		// Phong shading model
		fragColor = vec4(0);
		for (int i = 0; i < numLights; i++)
		{
			LightInfo lightInfo = lightInfos[i];
			vec3 ambientLightColor = lightInfo.ambient.rgb;
			vec3 diffuseLightColor = lightInfo.diffuse.rgb;
			vec3 specularLightColor = lightInfo.specular.rgb;

			// ambient light
			// more lights - less ambient impact
			float ambientStrength = (1 / numLights) * 0.1;
			vec3 ambient = ambientStrength * ambientLightColor * ambientMaterialComponent * ambientColor;

			//diffuse light
			vec3 norm = normalize(normal);
			vec3 lightDir = normalize(lightInfo.pos.rgb - fragment);
			float diffuseValue = max(dot(norm, lightDir), 0.0);
			vec3 diffuse = diffuseValue * diffuseLightColor * diffuseMaterialComponent * diffuseColor;

			// specular light
			float specularStrength = 0.5f;
			vec3 viewDir = normalize(viewPos - fragment);
			vec3 reflectedDir = reflect(-lightDir, norm);
			float specValue = pow(max(dot(viewDir, reflectedDir), 0.0), shininess);
			vec3 specular = specularStrength * specValue * specularLightColor * specularMaterialComponent * specularColor;

			if (lightInfo.type == g_directionalLightType)
			{
				float shadow = CalculateShadowValue(vec3(lightInfo.dir));
				fragColor += vec4((ambient + (1.0 - shadow) * (diffuse + specular)), 1.0);
			}
			else
			{
				// point light stuff. also needed for spot light 
				float distanceToFrag = length(lightInfo.pos.rgb - fragment);
				float attenuation = 1.0f / (lightInfo.constant + lightInfo.linear * distanceToFrag + 
																	lightInfo.quadratic * (distanceToFrag * distanceToFrag));
				// TODO: fix color banding
				// https://computergraphics.stackexchange.com/questions/3964/opengl-specular-shading-gradient-banding-issues
				ambient *= attenuation;
				diffuse *= attenuation;
				specular *= attenuation;
			
				// spot light stuff
				if (lightInfo.type == g_spotLightType)
				{
					float angle = dot(vec3(-lightInfo.dir), lightDir);
					if (bool(lightInfo.smoothSpotLight))
					{
						// outer < inner
						float epsilon = lightInfo.cutoff - lightInfo.outerCutoff;
						//float intensity = clamp((angle - lightInfo.outerCutoff) / epsilon, 0.f, 1.f);
						float intensity = smoothstep(lightInfo.outerCutoff, lightInfo.cutoff, angle);
						diffuse *= intensity;
						specular *= intensity;
					}
					else
					{
						// angle between direction from fragment to light and just light dir
						if (angle < lightInfo.cutoff)
						{
							// remove impact of diffuse + specular
							diffuse = vec3(0);
							specular = vec3(0);
							ambient *= 0.5;
						}
					}
				}
				fragColor += vec4((ambient + diffuse + specular), 1.0);
			}
		}
		fragColor *= color;
	}

	if (hasDefaultTexture)
	{
		fragColor *= texture(defaultTexture, uv);
	}
}
