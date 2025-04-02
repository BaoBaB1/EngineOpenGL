#pragma once

#include <glm/glm.hpp>

namespace fury
{
	struct Material
	{
		glm::vec3 ambient = glm::vec3(1.f);
		glm::vec3 diffuse = glm::vec3(1.f);
		glm::vec3 specular = glm::vec3(1.f);
		float shininess = 32.f;
		float alpha = 1.f;
	};
}
