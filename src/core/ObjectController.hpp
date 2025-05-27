#pragma once

#include "ITickable.hpp"
#include "ISerializable.hpp"
#include "utils/Macro.hpp"
#include <memory>

namespace fury
{
  class Object3D;

  class ObjectController : public ITickable, public ISerializable
  {
  public:
    enum class Type
    {
      ROTATION
    };
    OnlyMovable(ObjectController)
    [[nodiscard]] virtual ObjectController* clone() const = 0;
    Type get_type() const { return m_type; }
    void set_object(Object3D* obj) { m_obj = obj; }
    const Object3D* get_object() const { return m_obj; }
    void set_delta_time(float dt) { m_dt = dt; }
    bool is_enabled() const { return m_enabled; }
    void disable() { m_enabled = false; }
    void enable() { m_enabled = true; }
  protected:
    ObjectController(Type type) : m_type(type) {}
  protected:
    Object3D* m_obj = nullptr;
    Type m_type;
    bool m_enabled = true;
    float m_dt = 0.f;
  };
}
