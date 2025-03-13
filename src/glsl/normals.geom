#version 440 core

layout(points) in;
layout(line_strip, max_vertices = 2) out;

in VS_OUT
{
  vec3 normal;
} gs_in[];

//out vec4 color;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main()
{
  mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
  const float len_scaler = 5.f;
  // gl position is in local space !
  gl_Position = mvp * gl_in[0].gl_Position;
  EmitVertex();
  gl_Position = mvp * vec4(vec3(gl_in[0].gl_Position) + gs_in[0].normal / len_scaler, 1.0);
  EmitVertex();
  EndPrimitive();
}
