layout (location = 0) in vec2 aPos;

// ortho
uniform mat4 projectionMatrix;

void main()
{
  // -1 ?
  gl_Position = projectionMatrix * vec4(aPos, 0, 1);
}
