#version 440 core

out vec4 fragColor;

uniform vec3 lineColor;

void main()
{
    fragColor = vec4(lineColor, 1);
}
