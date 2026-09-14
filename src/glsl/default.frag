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
	int is_active;
};

layout (std430, binding = 2) readonly buffer Lights
{
	LightInfo lightInfos[];
};

out vec4 fragColor;

layout(location = 0) in vec3 normal;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 fragment;
layout(location = 3) in vec2 uv;
#ifdef APPLY_LIGHTING
	layout(location = 4) in vec4 fragPosDirectionalLightSpace;
#endif

#ifdef HAS_AMBIENT_TEXTURE
	layout(binding = 0) uniform sampler2D ambientTex;
#endif
#ifdef HAS_DIFFUSE_TEXTURE
	layout(binding = 1) uniform sampler2D diffuseTex;
#endif
#ifdef HAS_SPECULAR_TEXTURE
	layout(binding = 2) uniform sampler2D specularTex;
#endif
#ifdef APPLY_LIGHTING
	layout(binding = 3) uniform sampler2D shadowMap;
#endif

uniform int numLights;
uniform Material material;
uniform vec3 viewPos;
uniform vec4 overrideColor;

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


#ifdef APPLY_LIGHTING
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
#endif

void main()
{
#ifdef USE_VERTEX_COLOR
	fragColor = color;
#else 
  fragColor = overrideColor;
#endif

#ifdef HAS_DIFFUSE_TEXTURE
	fragColor *= texture(diffuseTex, uv);
#endif

#ifdef APPLY_LIGHTING
	if (numLights > 0)
	{
	#ifdef HAS_AMBIENT_TEXTURE
		vec3 ambientColor = texture(ambientTex, uv).rgb;
	#else 
		vec3 ambientColor = material.ambient;
	#endif
	#ifdef HAS_SPECULAR_TEXTURE
		vec3 specularColor = texture(specularTex, uv).rgb;
	#else
		vec3 specularColor = material.specular;
	#endif
		float shininess = material.shininess;
		float alpha = material.alpha;

		// Phong shading model
		vec4 lightColor = vec4(0);
		int numActiveLights = 0;
		for (int i = 0; i < numLights; i++)
		{
			LightInfo lightInfo = lightInfos[i];
			if (!bool(lightInfo.is_active))
				continue;
			numActiveLights++;
			vec3 ambientLightColor = lightInfo.ambient.rgb;
			vec3 diffuseLightColor = lightInfo.diffuse.rgb;
			vec3 specularLightColor = lightInfo.specular.rgb;
		
			// ambient light
			// more lights - less ambient impact
			float ambientStrength = (1 / numLights) * 0.1;
			vec3 ambient = ambientStrength * ambientLightColor * ambientColor;
		
			//diffuse light
			vec3 norm = normalize(normal);
			vec3 lightDir = normalize(lightInfo.pos.rgb - fragment);
			float diffuseValue = max(dot(norm, lightDir), 0.0);
			vec3 diffuse = diffuseValue * diffuseLightColor * material.diffuse;
		
			// specular light
			float specularStrength = 0.5f;
			vec3 viewDir = normalize(viewPos - fragment);
			vec3 reflectedDir = reflect(-lightDir, norm);
			float specValue = pow(max(dot(viewDir, reflectedDir), 0.0), shininess);
			vec3 specular = specularStrength * specValue * specularLightColor * specularColor;
		
			if (lightInfo.type == g_directionalLightType)
			{
				float shadow = CalculateShadowValue(vec3(lightInfo.dir));
				lightColor += vec4((ambient + (1.0 - shadow) * (diffuse + specular)), 1.0);
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
				lightColor += vec4((ambient + diffuse + specular), 1.0);
			}
		}
		// No active lights - no impact on fragment color
		if (numActiveLights > 0)
			fragColor *= lightColor;
	}
#endif
}
