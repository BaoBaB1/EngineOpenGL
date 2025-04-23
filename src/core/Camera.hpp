#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "ge/Ray.hpp"

namespace fury
{
  class Camera
  {
  public:
    enum Direction
    {
      FORWARD = 1,
      BACKWARD,
      LEFT,
      RIGHT,
      UP,
      DOWN
    };
    enum ProjectionMode : uint8_t
    {
      PERSPECTIVE,
      ORTHOGRAPHIC
    };
  public:
    Camera();
    glm::vec3 position() { return m_position; }
    glm::vec3 target() { return m_target; }
    const glm::vec3& position() const { return m_position; }
    const glm::vec3& target() const { return m_target; }
    float sensivity() const { return m_sensivity; }
    float speed() const { return m_base_speed; }
    bool freezed() const { return m_freezed; }
    Ray cast_ray(uint32_t x, uint32_t y) const;
    glm::vec2 get_screen_size() const { return m_screen_size; }
    glm::mat4 view_matrix() const;
    glm::mat4& get_projection_matrix() { return m_projection_mat[m_mode]; }
    const glm::mat4& get_projection_matrix() const { return m_projection_mat[m_mode]; }
    ProjectionMode get_projection_mode() const { return m_mode; }
    void set_projection_mode(ProjectionMode mode) { m_mode = mode; }
    void freeze() { m_freezed = true; }
    void unfreeze() { m_freezed = false; }
    void set_screen_size(const glm::vec2& screen_size);
    void set_position(const glm::vec3& position) { m_position = position; }
    void set_sensivity(float sensivity) { m_sensivity = sensivity; }
    void set_speed(float speed) { m_base_speed = speed; }
    void move(Direction direction);
    void scale_speed(float delta_time);
    void add_to_yaw_and_pitch(float x_offset, float y_offset);
    void update_camera_vectors();
    void look_at(const glm::vec3& position);
  private:
    float m_pitch;  // how much we are looking up or down
    float m_yaw;   // magnitute of looking left or right
    float m_base_speed; // speed from ctor
    float m_actual_speed; // speed according to deltatime
    float m_sensivity;
    bool m_freezed;
    glm::vec3 m_up;      // vector
    glm::vec3 m_target;  // vector
    glm::vec3 m_position; // point
    glm::mat4 m_projection_mat[2] = {};
    glm::vec2 m_screen_size;
    ProjectionMode m_mode = ProjectionMode::PERSPECTIVE;
  };

  static_assert(sizeof(Camera) == 200);
};
