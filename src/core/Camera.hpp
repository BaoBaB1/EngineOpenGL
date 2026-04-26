#pragma once

#include "ge/Ray.hpp"
#include "core/Entity.hpp"
#include "core/ITickable.hpp"
#include "Frustum.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <array>

namespace fury
{
  class Camera : public Entity
  {
  public:
    enum ProjectionMode : uint8_t
    {
      PERSPECTIVE,
      ORTHOGRAPHIC
    };
  public:
    FURY_REGISTER_DERIVED_CLASS(Camera, Entity)
    Camera();
    const glm::vec3& get_position() const;
    const glm::vec3& get_target() const { return m_target; }
    const glm::vec3& get_up() const { return m_up; }
    const glm::vec3& get_right() const { return m_right; }
    float get_sensivity() const { return m_sensivity; }
    float get_speed() const { return m_speed; }
    float get_fov() const { return m_fovy; }
    float get_aspect() const { return m_screen_size.x / m_screen_size.y; }
    float get_znear() const { return m_near_far.x; }
    float get_zfar() const { return m_near_far.y; }
    bool is_freezed() const { return m_freezed; }
    Ray cast_ray(uint32_t x, uint32_t y) const;
    glm::vec2 get_screen_size() const { return m_screen_size; }
    const glm::mat4& get_view_matrix() const;
    glm::mat4& get_view_matrix();
    glm::mat4& get_projection_matrix() { return m_projection_mat[m_mode]; }
    const glm::mat4& get_projection_matrix() const { return m_projection_mat[m_mode]; }
    ProjectionMode get_projection_mode() const { return m_mode; }
    Frustum get_frustum() const;
    void set_projection_mode(ProjectionMode mode) { m_mode = mode; }
    void freeze() { m_freezed = true; }
    void unfreeze() { m_freezed = false; }
    void set_screen_size(const glm::vec2& screen_size);
    void set_position(const glm::vec3& position);
    void set_sensivity(float sensivity) { m_sensivity = sensivity; }
    void set_speed(float speed) { m_speed = speed; }
    void add_to_yaw_and_pitch(float x_offset, float y_offset);
    void update_camera_vectors();
    void look_at(const glm::vec3& position);
    FURY_DECLARE_SERIALIZABLE_FIELDS(
      FURY_SERIALIZABLE_FIELD(1, &Camera::m_pitch),
      FURY_SERIALIZABLE_FIELD(2, &Camera::m_yaw),
      FURY_SERIALIZABLE_FIELD(3, &Camera::m_speed),
      FURY_SERIALIZABLE_FIELD(5, &Camera::m_sensivity),
      FURY_SERIALIZABLE_FIELD(6, &Camera::m_up),
      FURY_SERIALIZABLE_FIELD(7, &Camera::m_target),
      FURY_SERIALIZABLE_FIELD(8, &Camera::m_projection_mat),
      FURY_SERIALIZABLE_FIELD(9, &Camera::m_screen_size),
      FURY_SERIALIZABLE_FIELD(10, &Camera::m_mode)
    )
  private:
    std::array<glm::mat4, 2> m_projection_mat = {};
    mutable glm::mat4 m_cached_viewmatrix;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_target;
    glm::vec2 m_screen_size;
    glm::vec2 m_near_far = { 0.1f, 1000.f };
    float m_pitch;  // how much we are looking up or down
    float m_yaw;   // magnitute of looking left or right
    float m_speed; // speed from ctor
    float m_sensivity;
    float m_fovy = 45.f;
    ProjectionMode m_mode = ProjectionMode::PERSPECTIVE;
    bool m_freezed;
    mutable bool m_dirty = true;
  };

};
