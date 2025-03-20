#include "GPUBuffers.hpp"

void GPUBuffers::bind_all() 
{
  vao.bind();
  vbo.bind();
  ebo.bind();
}

void GPUBuffers::unbind_all() 
{
  vao.unbind();
  vbo.unbind();
  ebo.unbind();
}
