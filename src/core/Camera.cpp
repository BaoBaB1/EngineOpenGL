#include "Camera.hpp"
#include "core/SceneGraphManager.hpp"
#include "input/UserInputHandler.hpp"
#include "input/KeyboardHandler.hpp"
#include "input/CursorPositionHandler.hpp"
#include "core/IdGenerator.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace fury
{
  Camera::Camera() : Entity("Camera")
  {
    m_actual_speed = m_base_speed = 5.f;
    m_sensivity = 0.2f;
    m_yaw = -90.f;
    m_pitch = 0.f;
    m_up = glm::vec3(0.f, 1.f, 0.f);
    m_target = glm::vec3(0.f);
    m_freezed = false;
  }

  void Camera::move(Direction direction)
  {
    if (is_freezed())
      return;
    glm::vec3 pos = get_position();
    switch (direction)
    {
    case Direction::FORWARD:
      pos += m_actual_speed * m_target;
      break;
    case Direction::BACKWARD:
      pos -= m_actual_speed * m_target;
      break;
    case Direction::LEFT:
      pos -= glm::normalize(glm::cross(m_target, m_up)) * m_actual_speed;
      break;
    case Direction::RIGHT:
      pos += glm::normalize(glm::cross(m_target, m_up)) * m_actual_speed;
      break;
    case Direction::UP:
      pos += m_actual_speed * m_up;
      break;
    case Direction::DOWN:
      pos -= m_actual_speed * m_up;
      break;
    default:
      break;
    }
    set_position(pos);
  }

  const glm::vec3& Camera::get_position() const
  {
    return SceneGraphManager::get_entity_node<TransformationSceneNode>(m_id)->get_translation();
  }

  void Camera::set_position(const glm::vec3& position)
  {
    SceneGraphManager::get_entity_node<TransformationSceneNode>(m_id)->set_translation(position);
    m_dirty = true;
  }

  void Camera::set_screen_size(const glm::vec2& screen_size)
  {
    m_screen_size = screen_size;
    m_projection_mat[ProjectionMode::PERSPECTIVE] = glm::mat4(1.f);
    m_projection_mat[ProjectionMode::PERSPECTIVE] = glm::perspective(glm::radians(45.f), screen_size.x / screen_size.y, 0.1f, 100.f);
    const float aspect_ratio = screen_size.x / screen_size.y;
    const glm::vec2 ortho_wh = { 2.5f * aspect_ratio, 2.5f };
    m_projection_mat[ProjectionMode::ORTHOGRAPHIC] = glm::ortho(-ortho_wh.x, ortho_wh.x, -ortho_wh.y, ortho_wh.y, 0.1f, 100.f);
  }

  const glm::mat4& Camera::get_view_matrix() const
  {
    return const_cast<Camera*>(this)->get_view_matrix();
  }

  glm::mat4& Camera::get_view_matrix()
  {
    if (m_dirty)
    {
      auto node = SceneGraphManager::get_entity_node<TransformationSceneNode>(m_id);
      const glm::vec3& pos = node->get_translation();
      m_cached_viewmatrix = glm::lookAt(pos, pos + m_target, m_up);
      glm::quat q(glm::inverse(m_cached_viewmatrix));
      node->set_rotation(q);
      m_dirty = false;
    }
    return m_cached_viewmatrix;
  }

  Ray Camera::cast_ray(uint32_t x, uint32_t y) const
  {
    const glm::vec3 ray_nds = glm::vec3((2.f * x) / m_screen_size.x - 1.f, (2.f * y) / m_screen_size.y - 1.f, 1);
    const glm::vec4 ray_clip = glm::vec4(glm::vec2(ray_nds), -1, 1);
    const glm::vec3& pos = get_position();
    if (m_mode == PERSPECTIVE)
    {
      glm::vec4 ray_eye = glm::inverse(m_projection_mat[ProjectionMode::PERSPECTIVE]) * ray_clip;
      ray_eye.z = -1;
      ray_eye.w = 0;
      glm::vec3 ray_world_dir = glm::normalize(glm::inverse(get_view_matrix()) * ray_eye);
      return Ray(pos, ray_world_dir);
    }
    glm::vec4 world_pos = glm::inverse(m_projection_mat[ProjectionMode::ORTHOGRAPHIC] * get_view_matrix()) * ray_clip;
    world_pos /= world_pos.w;
    return Ray(world_pos, m_target);
  }

  void Camera::add_to_yaw_and_pitch(float x_offset, float y_offset)
  {
    if (is_freezed())
      return;
    m_yaw += x_offset * m_sensivity;
    m_pitch += y_offset * m_sensivity;
    update_camera_vectors();
  }

  void Camera::update_camera_vectors()
  {
    glm::vec3 dir;
    if (m_pitch > 89.0f)
      m_pitch = 89.0f;
    if (m_pitch < -89.0f)
      m_pitch = -89.0f;
    dir.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    dir.y = -sin(glm::radians(m_pitch)); // -sin ?
    dir.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_target = glm::normalize(dir);
    m_dirty = true;
  }

  void Camera::look_at(const glm::vec3& target) {
    const glm::vec3& pos = get_position();
    assert(target != pos);
    glm::vec3 camera_dir = glm::normalize(target - pos);
    m_yaw = std::atan2(camera_dir.z, camera_dir.x); // -x is possible also ?
    m_pitch = std::asin(-camera_dir.y);
    m_yaw = glm::degrees(m_yaw);
    m_pitch = glm::degrees(m_pitch);
    m_target = camera_dir;
    m_dirty = true;
  }

  void Camera::tick(float dt)
  {
    m_actual_speed = m_base_speed * dt;
  }

};
