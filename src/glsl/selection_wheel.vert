#version 440 core

layout (location = 0) in vec2 aPos;

//layout (std140, binding = 0) uniform CameraData
//{
//	mat4 viewMatrix;
//	mat4 projectionMatrix;
//} camData;

// ortho
uniform mat4 projectionMatrix;

void main()
{
  // -1 ?
  gl_Position = projectionMatrix * vec4(aPos, 0, 1);
}
