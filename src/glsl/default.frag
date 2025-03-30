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

uniform bool applyShading;
uniform bool hasDefaultTexture;
uniform bool hasAmbientTex;
uniform bool hasDiffuseTex;
uniform bool hasSpecularTex;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform sampler2D defaultTexture;
uniform sampler2D ambientTex;
uniform sampler2D diffuseTex;
uniform sampler2D specularTex;
uniform Material material;

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

		fragColor = vec4((diffuse + ambient + specular) * color.rgb, alpha);
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
