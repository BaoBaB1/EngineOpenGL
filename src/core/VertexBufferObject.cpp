#include "VertexBufferObject.hpp"

VertexBufferObject::VertexBufferObject() : VertexBufferObject(DEFAULT_VBO_SIZE)
{
}

VertexBufferObject::VertexBufferObject(size_t size) : OpenGLBuffer(GL_ARRAY_BUFFER, size)
{
}