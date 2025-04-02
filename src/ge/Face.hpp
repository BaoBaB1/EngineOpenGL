#pragma once

#include <vector>
#include <glad/glad.h>

namespace fury
{
  struct Face {

    Face() = default;
    Face(GLuint sz) { resize(sz); }
    Face(const std::initializer_list<GLuint>& indices);
    Face(const std::vector<GLuint>& indices);
    Face(const Face& other);
    Face& operator=(const Face& other);
    Face(Face&& other) noexcept;
    Face& operator=(Face&& other) noexcept;
    const uint32_t& operator[](GLuint idx) const { return data[idx]; }
    uint32_t& operator[](GLuint idx) { return data[idx]; }
    void resize(int size);
    ~Face();

    uint32_t size = 0;
    GLuint* data = nullptr;  // can use vector instead but, every Face will be increased by sizeof(vector)
  };
}
