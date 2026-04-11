#pragma once

#include "Event.hpp"
#include "ObjectChangeInfo.hpp"

namespace fury
{
  namespace global_state
  {
    inline Event<const ObjectChangeInfo&> g_on_object_change;
  }
}
