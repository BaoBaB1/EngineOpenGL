#version 440 core

out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform bool isSingleChannel;

void main()
{ 
  if (!isSingleChannel)
  {
    FragColor = texture(screenTexture, TexCoords);
  }
  else
  {
    float depth = texture(screenTexture, TexCoords).r;
    FragColor = vec4(vec3(depth), 1);
  }
}
