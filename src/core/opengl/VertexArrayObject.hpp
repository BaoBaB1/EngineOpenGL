#pragma once

#include "OpenGLObject.hpp"

class VertexArrayObject : public OpenGLObject
{
public:
  OnlyMovable(VertexArrayObject)
  VertexArrayObject();
  ~VertexArrayObject();
  void link_attrib(GLuint layout, GLuint num_components, GLenum type, GLsizei stride, void* offset);
  void bind() const override;
  void unbind() const override;
};
