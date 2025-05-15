#version 440 core

struct Material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
	float alpha;
};

out vec4 fragColor;

in vec3 normal;
in vec4 color;
in vec3 fragment;
in vec2 uv;
in vec4 fragPosLightSpace;

uniform bool applyShading;
uniform bool hasDefaultTexture;
uniform bool hasAmbientTex;
uniform bool hasDiffuseTex;
uniform bool hasSpecularTex;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 lightDirGlobal;
uniform sampler2D defaultTexture;
uniform sampler2D ambientTex;
uniform sampler2D diffuseTex;
uniform sampler2D specularTex;
uniform sampler2D shadowMap;
uniform Material material;

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


float CalculateShadowValue()
{
	// perform perspective divide and scale coord to [-1, 1] range
	vec3 ndc = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// scale to range [0, 1] because depth values in shadow map are in range [0, 1]
	ndc = ndc * 0.5 + 0.5;
	// everything that is out of shadow map or if normal is facing away from light
	float dp = dot(normal, -normalize(lightDirGlobal));
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
	}
	if (hasSpecularTex)
	{
		specularColor = texture(specularTex, uv).rgb;
	}

	if (applyShading)
	{
		vec3 ambientMaterialComponent = material.ambient;
		vec3 diffuseMaterialComponent = material.diffuse;
		vec3 specularMaterialComponent = material.specular;
		float shininess = material.shininess;
		float alpha = material.alpha;

		// Phong shading model

		// ambient light
		float ambientStrength = 0.2f;
		vec3 ambient = ambientStrength * lightColor * ambientMaterialComponent * ambientColor;

		//diffuse light
		vec3 norm = normalize(normal);
		vec3 lightDir = normalize(lightPos - fragment);
		float diffuseValue = max(dot(norm, lightDir), 0.0);
		vec3 diffuse = diffuseValue * lightColor * diffuseMaterialComponent * diffuseColor;

		// specular light
		float specularStrength = 0.5f;
		vec3 viewDir = normalize(viewPos - fragment);
		vec3 reflectedDir = reflect(-lightDir, norm);
		float specValue = pow(max(dot(viewDir, reflectedDir), 0.0), shininess);
		vec3 specular = specularStrength * specValue * lightColor * specularMaterialComponent * specularColor;

		float shadow = CalculateShadowValue();
		fragColor = vec4((ambient + (1.0 - shadow) * (diffuse + specular)) * color.rgb, 1.0);
	}
	else
	{
		fragColor = color;
	}

	if (hasDefaultTexture)
	{
		fragColor *= texture(defaultTexture, uv);
	}
}
