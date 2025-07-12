#version 440 core

layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 uv;

out vec2 texCoords;

// ortho
uniform mat4 projectionMatrix;

void main()
{
  gl_Position = projectionMatrix * vec4(pos, 0, 1);
  texCoords = uv;
}
