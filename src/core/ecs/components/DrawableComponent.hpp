#pragma once

#include "core/Common.hpp"
#include "core/CommonEnums.hpp"
#include "core/Macros.hpp"
#include "core/Serialization.hpp"
#include "core/BoundingBox.hpp"
#include "core/Mesh.hpp"
#include "core/Material.hpp"
#include "core/MeshNode.hpp"

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <string>
#include <vector>

namespace fury
{
  // TODO: remove?
  enum DrawableFlags
  {
    HAS_SURFACE = (1 << 0)
  };

  struct RenderConfig
  {
    int mode = GL_TRIANGLES;
    bool use_indices = true;
    bool use_color_from_vertex_attribute = false;
  };

  namespace ecs
  {
    struct DrawableComponent
    {
      FURY_REGISTER_COMPONENT(DrawableComponent)
      FURY_DECLARE_SERIALIZABLE_FIELDS(FURY_SERIALIZABLE_FIELD(1, &DrawableComponent::name),
                                       FURY_SERIALIZABLE_FIELD(2, &DrawableComponent::mesh_nodes),
                                       FURY_SERIALIZABLE_FIELD(3, &DrawableComponent::bbox),
                                       FURY_SERIALIZABLE_FIELD(4, &DrawableComponent::color),
                                       FURY_SERIALIZABLE_FIELD(5, &DrawableComponent::render_config),
                                       FURY_SERIALIZABLE_FIELD(6, &DrawableComponent::flags),
                                       FURY_SERIALIZABLE_FIELD(7, &DrawableComponent::shading_mode))
      std::string name = "Entity";
      std::vector<MeshNode> mesh_nodes;
      BoundingBox bbox;
      glm::vec4 color = glm::vec4(1.f);
      RenderConfig render_config;
      uint32_t flags = 0;
      ShadingMode shading_mode = ShadingMode::NO_SHADING;
    };
  } // namespace ecs
} // namespace fury
