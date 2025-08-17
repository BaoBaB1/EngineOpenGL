#version 440 core

layout (std140, binding = 0) uniform CameraData
{
  mat4 viewMatrix;
  mat4 projectionMatrix;
} camData;

// 2 triangles that cover full screen
vec3 gridPlane[6] = vec3[](
    vec3(1, 1, 0), vec3(-1, 1, 0), vec3(-1, -1, 0),
    vec3(-1, -1, 0), vec3(1, -1, 0), vec3(1, 1, 0)
);

out vec3 nearPoint;
out vec3 farPoint;

vec3 UnprojectPoint(vec3 pnt)
{
  // convert from clip space to world space
  mat4 viewInv = inverse(camData.viewMatrix);
  mat4 projInv = inverse(camData.projectionMatrix);
  vec4 worldSpacePoint = viewInv * projInv * vec4(pnt, 1);
  return worldSpacePoint.xyz / worldSpacePoint.w;
}

void main()
{
  vec3 pos = gridPlane[gl_VertexID];
  // those are interpolated among pixels of triangle
  nearPoint = UnprojectPoint(vec3(pos.x, pos.y, -1));
  farPoint = UnprojectPoint(vec3(pos.x, pos.y, 1));
  // clip space
  gl_Position = vec4(pos, 1);
}
