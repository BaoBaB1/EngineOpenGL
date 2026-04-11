#pragma once

#include <glm/glm.hpp>

namespace fury
{
  class TransformationSceneNode;
  class Object3D;

  struct ObjectChangeInfo
  {
    Object3D* object = nullptr;
    TransformationSceneNode* new_transform = nullptr;
    bool is_shading_mode_change = false;
    bool is_color_change = false;
  };
}
