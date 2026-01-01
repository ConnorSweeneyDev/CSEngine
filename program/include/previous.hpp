#pragma once

#include "graphics.hpp"
#include "state.hpp"

namespace cse::help
{
  struct window_previous
  {
    window_previous();
    window_previous(const window_state &state_);

    window_state state{};
  };

  struct camera_previous
  {
    camera_state state{};
    camera_graphics graphics{};
  };

  struct object_previous
  {
    object_previous();
    object_previous(const object_state &state_, const object_graphics &graphics_);

    object_state state{};
    object_graphics graphics{};
  };
}
