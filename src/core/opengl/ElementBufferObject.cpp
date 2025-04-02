#include "ElementBufferObject.hpp"

ElementBufferObject::ElementBufferObject() : OpenGLBuffer(GL_ELEMENT_ARRAY_BUFFER)
{
}

ElementBufferObject::ElementBufferObject(size_t size) : OpenGLBuffer(GL_ELEMENT_ARRAY_BUFFER, size)
{
}
