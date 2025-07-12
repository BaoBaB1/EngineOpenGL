#version 440 core

out vec4 fragColor;

uniform vec3 color;
uniform bool isSlotSelected;

void main()
{
  const float opacity = 0.5;
  if (!isSlotSelected)
  {
    fragColor = vec4(color, opacity);
  }
  else
  {
    vec3 final = (color + vec3(1)) / 2;
    fragColor = vec4(final, opacity);
  }
}
