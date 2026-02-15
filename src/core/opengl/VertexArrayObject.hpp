#pragma once

#include "OpenGLObject.hpp"

namespace fury
{
  class VertexArrayObject : public OpenGLObject
  {
  public:
    VertexArrayObject();
    ~VertexArrayObject();
    void link_attrib(GLuint layout, GLuint num_components, GLenum type, GLsizei stride, void* offset);
    void bind() const override;
    void unbind() const override;
  };
}
