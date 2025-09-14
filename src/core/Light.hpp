#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace fury
{
  class Object3D;

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

    // attenuation info for point light
    float constant = 1.f;
    float linear = 0.14f;
    float quadratic = 0.07f;

    // cutoff for spot light (cos(angle))
    float cutoff = 0.9654321;
    float outerCutoff = 0.9;
    int32_t smoothSpotLight = true;

    int32_t type = -1;
    int32_t shadow_matrix_buffer_index = -1;
  };
  static_assert(sizeof(LightDescription) % 16 == 0);

  class Light
  {
  public:
    // keep it here for now
    inline static std::vector<glm::mat4> shadow_matrices;
    Light() = default;
    Light(const LightDescription& desc) : m_desc(desc) {}
    const LightDescription& get_description() const { return m_desc; }
    LightDescription& get_description() { return m_desc; }
    void set_description(const LightDescription& desc) { m_desc = desc; }
    bool is_valid() const { return m_desc.type >= 0 && m_desc.type < LightType::LAST_ITEM; }
    void disable() { m_enabled = false; }
    void enable() { m_enabled = true; }
    void update_position(const glm::vec3& new_pos) { m_desc.position = glm::vec4(new_pos, 1); }
    void update_direction(const glm::vec3& new_dir) { m_desc.dir = glm::vec4(new_dir, 1); }
    bool is_enabled() const { return m_enabled; }
    LightType get_type() const { return static_cast<LightType>(m_desc.type); }
    const Object3D* get_parent() const { return m_parent; }
    void set_parent(Object3D* parent) { m_parent = parent; }
    void set_shadow_matrix(const glm::mat4& mat);
    glm::mat4 get_shadow_matrix_safe() const;
    const glm::mat4& get_shadow_matrix() const;
  private:
    LightDescription m_desc;
    Object3D* m_parent = nullptr;
    bool m_enabled = true;
  };
}
