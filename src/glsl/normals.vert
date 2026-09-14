layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;

out VS_OUT
{
  vec3 normal;
  mat4 modelMat;
} vs_out;

layout (std430, binding = 1) readonly buffer ModelMatrices
{
  mat4 modelMatrices[];
};

layout (std430, binding = 2) readonly buffer ModelMatricesIndices
{
  int modelMatricesIndices[];
};

void main()
{
    gl_Position = vec4(pos, 1.0);
    vs_out.normal = normal;
    vs_out.modelMat = modelMatrices[modelMatricesIndices[gl_DrawID]];
}
