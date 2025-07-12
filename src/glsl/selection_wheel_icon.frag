#version 440 core

out vec4 fragColor;
  
in vec2 texCoords;

uniform sampler2D iconTex;

void main()
{
  fragColor = texture(iconTex, texCoords);
}
