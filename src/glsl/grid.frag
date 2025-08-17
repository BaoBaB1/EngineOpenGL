#version 440 core

out vec4 fragColor;

in vec3 nearPoint;
in vec3 farPoint;

layout (std140, binding = 0) uniform CameraData
{
  mat4 viewMatrix;
  mat4 projectionMatrix;
} camData;

vec4 grid(vec3 fragPos, float scale)
{
  // I have no idea what this math does, but it works
  // https://asliceofrendering.com/scene%20helper/2020/01/05/InfiniteGrid/
  vec2 coord = fragPos.xz * scale; // use the scale variable to set the distance between the lines
  vec2 derivative = fwidth(coord);
  vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
  float line = min(grid.x, grid.y);
  float minimumz = min(derivative.y, 1);
  float minimumx = min(derivative.x, 1);
  vec4 color = vec4(0.2, 0.2, 0.2, 1.0 - min(line, 1.0));
  // z axis
  if(fragPos.x > -0.1 * minimumx && fragPos.x < 0.1 * minimumx)
      color.z = 1.0;
  // x axis
  if(fragPos.z > -0.1 * minimumz && fragPos.z < 0.1 * minimumz)
      color.x = 1.0;
  return color;
}

float computeDepth(vec3 pos)
{
  vec4 clip_space_pos = camData.projectionMatrix * camData.viewMatrix * vec4(pos.xyz, 1.0);
  // [-1, 1]
  float d = clip_space_pos.z / clip_space_pos.w;
  // map to [0, 1]
  return (d + 1) / 2;
}

void main()
{
  // cast ray from near point to far point and check if it hits the floor (y = 0)
  float t = -nearPoint.y / (farPoint.y - nearPoint.y);
  vec3 rayHit = nearPoint + t * (farPoint - nearPoint);
  // compute depth for each world space point manually, because all grid points are procedurally generated, thus having no depth
  gl_FragDepth = computeDepth(rayHit);
  fragColor = grid(rayHit, 5) * float(t > 0);
}
