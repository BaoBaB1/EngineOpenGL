#version 440 core

layout (location = 0) in vec3 aPos;

layout (std140, binding = 0) uniform CameraData
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
} camData;

out vec3 TexCoords;

void main()
{
    TexCoords = aPos;
    // remove translation
    mat4 vm = mat4(mat3(camData.viewMatrix));
    // model matrix is not needed, because skybox is at the origin
    vec4 pos = (camData.projectionMatrix * vm) * vec4(aPos, 1.0);
    // set z value to be always 1(max depth test value)
    gl_Position = pos.xyww;
}
