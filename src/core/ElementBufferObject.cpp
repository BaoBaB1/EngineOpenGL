#include "ElementBufferObject.hpp"

ElementBufferObject::ElementBufferObject() : ElementBufferObject(DEFAULT_EBO_SIZE)
{
}

ElementBufferObject::ElementBufferObject(size_t size) : OpenGLBuffer(GL_ELEMENT_ARRAY_BUFFER, size)
{
}
