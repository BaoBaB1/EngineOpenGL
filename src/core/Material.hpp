#pragma once

#include <glm/glm.hpp>

struct Material
{
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float shininess = 32.f;
	float alpha = 1.f;
};
