#include "ElementBufferObject.hpp"

ElementBufferObject::ElementBufferObject()
{
  glGenBuffers(1, id_ref());
}

void ElementBufferObject::set_data(const void* indices, size_t size_in_bytes)
{
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_in_bytes, indices, GL_STATIC_DRAW);
}

void ElementBufferObject::bind() const
{
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id);
}

void ElementBufferObject::unbind() const
{
  // MAKE SURE TO UNBIND IT AFTER UNBINDING THE VAO, as the EBO is linked in the VAO
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

ElementBufferObject::~ElementBufferObject()
{
  glDeleteBuffers(1, id_ref());
}
