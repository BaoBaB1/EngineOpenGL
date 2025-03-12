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
uniform bool hasMaterial;
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
	if (applyShading)
	{
		vec3 ambientComponent = vec3(1);
		vec3 diffuseComponent = vec3(1);
		vec3 specularComponent = vec3(1);
		float shininess = 32.f;
		float alpha = color.a;

		if (hasMaterial)
		{
			ambientComponent = material.ambient;
			diffuseComponent = material.diffuse;
			specularComponent = material.specular;
			shininess = material.shininess;
			alpha = material.alpha;
		}

		// Phong shading model

		// ambient light
		float ambientStrength = 0.2f;
		vec3 ambient = ambientStrength * lightColor * ambientComponent;

		//diffuse light
		vec3 norm = normalize(normal);
		vec3 lightDir = normalize(lightPos - fragment);
		float diffuseValue = max(dot(norm, lightDir), 0.0);
		vec3 diffuse = diffuseValue * lightColor * diffuseComponent;

		// specular light
		float specularStrength = 0.5f;
		vec3 viewDir = normalize(viewPos - fragment);
		vec3 reflectedDir = reflect(-lightDir, norm);
		float specValue = pow(max(dot(viewDir, reflectedDir), 0.0), shininess);
		vec3 specular = specularStrength * specValue * lightColor * specularComponent;

		fragColor = vec4((diffuse + ambient + specular) * color.rgb, alpha);

		if (hasAmbientTex)
		{
			fragColor *= texture(ambientTex, uv);
		}
		if (hasDiffuseTex)
		{
			fragColor *= texture(diffuseTex, uv);
		}
		if (hasSpecularTex)
		{
			fragColor += texture(specularTex, uv) * vec4(specular, 1);
		}
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
