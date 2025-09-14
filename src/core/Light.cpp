#include "Light.hpp"
#include "Logger.hpp"

namespace fury
{
  void Light::set_shadow_matrix(const glm::mat4& mat)
  {
    shadow_matrices.push_back(mat);
    m_desc.shadow_matrix_buffer_index = shadow_matrices.size() - 1;
  }

  glm::mat4 Light::get_shadow_matrix_safe() const
  {
    if (m_desc.shadow_matrix_buffer_index == -1)
    {
      Logger::error("Getting shadow matrix for light with no set matrix.");
      return glm::mat4(1.f);
    }
    return get_shadow_matrix();
  }

  const glm::mat4& Light::get_shadow_matrix() const
  {
    return shadow_matrices[m_desc.shadow_matrix_buffer_index];
  }
}
