#version 440 core

out vec4 fragColor;

uniform vec3 normalColor;

void main()
{
    fragColor = vec4(normalColor, 1);
}
