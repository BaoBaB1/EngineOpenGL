#version 440 core

out uvec4 FragColor;
  
uniform uint objectIndex;

void main()
{
    FragColor = uvec4(objectIndex, 50, 20, 1);
}
