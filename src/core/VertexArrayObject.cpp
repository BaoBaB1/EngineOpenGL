#include "VertexArrayObject.hpp"


VertexArrayObject::VertexArrayObject() {
  glGenVertexArrays(1, &m_id);
}

void VertexArrayObject::link_attrib(GLuint layout, GLuint num_components, GLenum type, GLsizei stride, void* offset) {
  // Configure the Vertex Attribute so that OpenGL knows how to read the VBO
  glVertexAttribPointer(layout, num_components, type, GL_FALSE, stride, offset);
  // Enable the Vertex Attribute so that OpenGL knows to use it
  glEnableVertexAttribArray(layout);
}

void VertexArrayObject::bind() {
  glBindVertexArray(m_id);
}

void VertexArrayObject::unbind() {
  glBindVertexArray(0);
}

VertexArrayObject::~VertexArrayObject() {
  glDeleteVertexArrays(1, &m_id);
}
