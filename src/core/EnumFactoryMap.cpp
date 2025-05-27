#include "EnumFactoryMap.hpp"
#include "RotationController.hpp"

namespace fury
{
  auto init = []()
    {
      EnumFactoryMap<ObjectController::Type, ObjectController> obj_c_mp;
      obj_c_mp.add_entry<RotationController>(ObjectController::Type::ROTATION);
      return 1;
    }();
}
