#pragma once

#include "Common.hpp"
#include "Macros.hpp"
#include "opengl/Texture2D.hpp"
#include <glm/glm.hpp>

namespace fury
{
  struct Material
  {
    FURY_DECLARE_SERIALIZABLE_FIELDS(
        FURY_SERIALIZABLE_FIELD(1, &Material::ambient), FURY_SERIALIZABLE_FIELD(2, &Material::diffuse),
        FURY_SERIALIZABLE_FIELD(3, &Material::specular), FURY_SERIALIZABLE_FIELD(4, &Material::shininess),
        FURY_SERIALIZABLE_FIELD(5, &Material::alpha), FURY_SERIALIZABLE_FIELD(6, &Material::ambient_tex),
        FURY_SERIALIZABLE_FIELD(7, &Material::diffuse_tex), FURY_SERIALIZABLE_FIELD(8, &Material::specular_tex))

    glm::vec3 ambient = glm::vec3(1.f);
    glm::vec3 diffuse = glm::vec3(1.f);
    glm::vec3 specular = glm::vec3(1.f);
    float shininess = 32.f;
    float alpha = 1.f;

    AssetHandle<Texture2D> ambient_tex;
    AssetHandle<Texture2D> diffuse_tex;
    AssetHandle<Texture2D> specular_tex;
  };
} // namespace fury
