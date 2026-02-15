#pragma once

#include "core/Macros.hpp"
#include <glad/glad.h>

namespace fury
{
  template<typename T>
  struct OpenGLIdWrapper
  {
    OpenGLIdWrapper() = default;
    OpenGLIdWrapper(const OpenGLIdWrapper& other) = delete;
    OpenGLIdWrapper(OpenGLIdWrapper&& other) noexcept;
    OpenGLIdWrapper& operator=(const OpenGLIdWrapper& other) = delete;
    OpenGLIdWrapper& operator=(OpenGLIdWrapper&& other) noexcept;
    operator T() const { return id; }
    T id = 0;
  };

  template<typename T>
  OpenGLIdWrapper<T>::OpenGLIdWrapper(OpenGLIdWrapper<T>&& other) noexcept
  {
    id = other.id;
    other.id = 0;
  }

  template<typename T>
  OpenGLIdWrapper<T>& OpenGLIdWrapper<T>::operator=(OpenGLIdWrapper<T>&& other) noexcept
  {
    if (this != &other)
    {
      id = other.id;
      other.id = 0;
    }
    return *this;
  }

  class OpenGLObject
  {
  public:
    FURY_OnlyMovable(OpenGLObject)
    OpenGLObject() = default;
    virtual ~OpenGLObject() = default;
    virtual void bind() const = 0;
    virtual void unbind() const = 0;
    GLuint id() const { return m_id; }
  protected:
    GLuint* id_ref() { return &m_id.id; }
    OpenGLIdWrapper<GLuint> m_id;
  };
}
