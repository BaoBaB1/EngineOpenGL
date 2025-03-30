#version 440 core

#extension ARB_shader_draw_parameters : require

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;

out VS_OUT
{
  vec3 normal;
  mat4 modelMat;
} vs_out;

layout (std430, binding = 1) buffer ModelMatrices
{
  mat4 modelMatrices[];
};

void main()
{
    gl_Position = vec4(pos, 1.0);
    vs_out.normal = normal;
    vs_out.modelMat = modelMatrices[gl_DrawIDARB];
}
