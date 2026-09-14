layout (std140, binding = 0) uniform CameraData
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
} camData;

struct AABBBound {
  vec3 minp;
  vec3 maxp;
}; 

layout(std430, binding = 1) readonly buffer AABBBounds
{
  AABBBound aabbBounds[];
};

const vec3 cubeVertices[8] = vec3[8](
  vec3(-0.5, -0.5, 0.5),  vec3(0.5, -0.5, 0.5),  vec3(0.5, 0.5, 0.5),  vec3(-0.5, 0.5, 0.5),
  vec3(-0.5, -0.5, -0.5), vec3(0.5, -0.5, -0.5), vec3(0.5, 0.5, -0.5), vec3(-0.5, 0.5, -0.5)
);

const int cubeLinesIndices[24] = int[24](
  0, 1, 1, 2, 2, 3, 3, 0, // front
  4, 5, 5, 6, 6, 7, 7, 4, // back
  0, 4, 3, 7, 1, 5, 2, 6
);

void main()
{
  const AABBBound bounds = aabbBounds[gl_InstanceID];
  const vec3 unitCubeVertex = cubeVertices[cubeLinesIndices[gl_VertexID]];
  const vec3 extents = bounds.maxp - bounds.minp;
  const vec3 cubeCenter = (bounds.maxp + bounds.minp) * 0.5;
  const vec3 cubeVertexWorld = (unitCubeVertex * extents) + cubeCenter;
  gl_Position = (camData.projectionMatrix * camData.viewMatrix) * vec4(cubeVertexWorld, 1.0);
}
