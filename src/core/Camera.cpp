#include "Camera.hpp"
#include "Frustum.hpp"
#include "core/SceneGraphManager.hpp"
#include "core/IdGenerator.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "RenderPass.hpp"

namespace fury
{
  const glm::vec3 world_up = glm::vec3(0, 1, 0);

  Camera::Camera() : Entity("Camera")
  {
    m_speed = 5.f;
    m_sensivity = 0.2f;
    m_yaw = -90.f;
    m_pitch = 0.f;
    m_up = world_up;
    m_target = glm::vec3(0.f);
    m_freezed = false;
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
    m_projection_mat[ProjectionMode::PERSPECTIVE] = glm::perspective(glm::radians(m_fovy), get_aspect(), get_znear(), get_zfar());
    const glm::vec2 ortho_wh = { 2.5f * get_aspect(), 2.5f};
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
      m_cached_viewmatrix = glm::lookAt(pos, pos + m_target, world_up);
      m_right = glm::normalize(glm::cross(m_target, world_up));
      m_up = glm::cross(m_right, m_target);
      glm::quat q(glm::inverse(m_cached_viewmatrix));
      node->set_rotation(q);
      m_dirty = false;
    }
    return m_cached_viewmatrix;
  }

  Frustum Camera::get_frustum() const
  {
    Frustum fr;
    const float zfar = get_zfar();
    const float znear = get_znear();
    // float zfar = 10;
    const float fov = glm::radians(m_fovy);
    // tg = a / b => a = tg * b
    // aspect = w / h => w = aspect * h
    const float half_far_height = std::tan(fov / 2) * zfar;
    const float half_far_width = get_aspect() * half_far_height;
    const glm::vec3& cam_pos = get_position();
    const glm::vec3 center_far = cam_pos + m_target * zfar;
    const glm::vec3 center_near = cam_pos + m_target * znear;
    const glm::vec3 far_top_left = center_far + m_up * half_far_height - m_right * half_far_width;
    const glm::vec3 far_top_right = center_far + m_up * half_far_height + m_right * half_far_width;
    const glm::vec3 far_bottom_left = center_far - m_up * half_far_height - m_right * half_far_width;
    const glm::vec3 far_bottom_right = center_far - m_up * half_far_height + m_right * half_far_width;
    fr.near.normal = m_target;
    fr.near.distance = -glm::dot(fr.near.normal, center_near);
    fr.far.normal = -m_target;
    fr.far.distance = -glm::dot(fr.far.normal, center_far);
    fr.left.normal = glm::normalize(glm::cross(far_bottom_left - cam_pos, far_top_left - cam_pos));
    // Plane equation: ax + by + cz + d = 0
    fr.left.distance = -glm::dot(fr.left.normal, far_bottom_left);
    fr.right.normal = glm::normalize(glm::cross(far_top_right - cam_pos, far_bottom_right - cam_pos));
    fr.right.distance = -glm::dot(fr.right.normal, far_bottom_right);
    fr.top.normal = glm::normalize(glm::cross(far_top_left -cam_pos, far_top_right - cam_pos));
    fr.top.distance = -glm::dot(fr.top.normal, far_top_left);
    fr.bottom.normal = glm::normalize(glm::cross(far_bottom_right - cam_pos, far_bottom_left - cam_pos));
    fr.bottom.distance = -glm::dot(fr.bottom.normal, far_bottom_left);

    fr.debug_lines.reserve(18);
    // far rect
    auto& l1 = fr.debug_lines.emplace_back();
    auto& l2 = fr.debug_lines.emplace_back();
    auto& l3 = fr.debug_lines.emplace_back();
    auto& l4 = fr.debug_lines.emplace_back();
    l1.first = far_bottom_left;
    l1.second = far_bottom_right;
    l2.first = far_bottom_left;
    l2.second = far_top_left;
    l3.first = far_bottom_right;
    l3.second = far_top_right;
    l4.first = far_top_left;
    l4.second = far_top_right;
    // near rect
    const float half_near_height = std::tan(fov / 2) * znear;
    const float half_near_width = get_aspect() * half_near_height;
    const glm::vec3 near_top_left = center_near + m_up * half_near_height - m_right * half_near_width;
    const glm::vec3 near_top_right = center_near + m_up * half_near_height + m_right * half_near_width;
    const glm::vec3 near_bottom_left = center_near - m_up * half_near_height - m_right * half_near_width;
    const glm::vec3 near_bottom_right = center_near - m_up * half_near_height + m_right * half_near_width;
    auto& l5 = fr.debug_lines.emplace_back();
    auto& l6 = fr.debug_lines.emplace_back();
    auto& l7 = fr.debug_lines.emplace_back();
    auto& l8 = fr.debug_lines.emplace_back();
    l5.first = near_bottom_left;
    l5.second = near_bottom_right;
    l6.first = near_bottom_left;
    l6.second = near_top_left;
    l7.first = near_bottom_right;
    l7.second = near_top_right;
    l8.first = near_top_left;
    l8.second = near_top_right;
    // sides
    auto& l9 = fr.debug_lines.emplace_back();
    auto& l10 = fr.debug_lines.emplace_back();
    auto& l11 = fr.debug_lines.emplace_back();
    auto& l12 = fr.debug_lines.emplace_back();
    l9.first = near_bottom_left;
    l9.second = far_bottom_left;
    l10.first = near_top_left;
    l10.second = far_top_left;
    l11.first = near_bottom_right;
    l11.second = far_bottom_right;
    l12.first = near_top_right;
    l12.second = far_top_right;
#if 0
    // normals
    const glm::vec3 center_left = ((((near_bottom_left + near_top_left) / 2.f) + ((far_bottom_left + far_top_left) / 2.f)) / 2.f);
    const glm::vec3 center_right = ((((near_bottom_right + near_top_right) / 2.f) + ((far_bottom_right + far_top_right) / 2.f)) / 2.f);
    const glm::vec3 center_top = ((((near_top_left + near_top_right) / 2.f) + ((far_top_left + far_top_right) / 2.f)) / 2.f);
    const glm::vec3 center_bottom = ((((near_bottom_left + near_bottom_right) / 2.f) + ((far_bottom_left + far_bottom_right) / 2.f)) / 2.f); 
    auto& l13 = fr.debug_lines.emplace_back();
    auto& l14 = fr.debug_lines.emplace_back();
    auto& l15 = fr.debug_lines.emplace_back();
    auto& l16 = fr.debug_lines.emplace_back();
    auto& l17 = fr.debug_lines.emplace_back();
    auto& l18 = fr.debug_lines.emplace_back();
    l13.first = center_near;
    l13.second = center_near + fr.near.normal * 3.f;
    l14.first = center_far;
    l14.second = center_far + fr.far.normal * 3.f;
    l15.first = center_left;
    l15.second = center_left + fr.left.normal * 3.f;
    l16.first = center_right;
    l16.second = center_right + fr.right.normal * 3.f;
    l17.first = center_top;
    l17.second = center_top + fr.top.normal * 3.f;
    l18.first = center_bottom;
    l18.second = center_bottom + fr.bottom.normal * 3.f;
    // camera normals
    auto& right_norm = fr.debug_lines.emplace_back();
    right_norm.first = cam_pos;
    right_norm.second = cam_pos + m_right * 3.f;
    auto& up_norm = fr.debug_lines.emplace_back();
    up_norm.first = cam_pos;
    up_norm.second = cam_pos + m_up * 3.f;
#endif

    return fr;
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
};
