#pragma once

#include "Entity.hpp"
#include <glm/glm.hpp>

namespace fury
{
  enum LightType : int32_t
  {
    DIRECTIONAL,
    POINT,
    SPOT,
    LAST_ITEM
  };

  struct LightDescription
  {
    // match GLSL 430 layout padding
    glm::vec4 position;
    glm::vec4 dir;
    glm::vec4 ambient = glm::vec4(1.f);
    glm::vec4 diffuse = glm::vec4(1.f);
    glm::vec4 specular = glm::vec4(1.f);
    // matrix for transformation to light space
    glm::mat4 shadow_matrix;

    // attenuation info for point light
    float constant = 1.f;
    float linear = 0.14f;
    float quadratic = 0.07f;

    // cutoff for spot light (cos(angle))
    float cutoff = 0.9654321f;
    float outerCutoff = 0.9f;
    int32_t smoothSpotLight = true;

    int32_t type = -1;
    int32_t pad;
    //int32_t shadow_matrix_buffer_index = -1;
  };
  static_assert(sizeof(LightDescription) % 16 == 0);

  class Light : public Entity
  {
  public:
    FURY_REGISTER_DERIVED_CLASS(Light, Entity)
    Light(const char* name = "Light") : Entity(name) {}
    Light(const LightDescription& desc) : m_desc(desc) {}
    const LightDescription& get_description() const { return m_desc; }
    LightDescription& get_description() { return m_desc; }
    void set_description(const LightDescription& desc) { m_desc = desc; }
    void set_position(const glm::vec3& new_pos) { m_desc.position = glm::vec4(new_pos, 1); }
    void set_direction(const glm::vec3& new_dir) { m_desc.dir = glm::vec4(new_dir, 1); }
    bool is_valid() const { return m_desc.type >= 0 && m_desc.type < LightType::LAST_ITEM; }
    void disable() { m_enabled = false; }
    void enable() { m_enabled = true; }
    bool is_enabled() const { return m_enabled; }
    LightType get_type() const { return static_cast<LightType>(m_desc.type); }
    FURY_DECLARE_SERIALIZABLE_FIELDS(
      FURY_SERIALIZABLE_FIELD(1, &Light::m_desc),
      FURY_SERIALIZABLE_FIELD(2, &Light::m_enabled)
    )
  private:
    LightDescription m_desc;
    bool m_enabled = true;
  };
}
