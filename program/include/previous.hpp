#pragma once

#include "graphics.hpp"
#include "state.hpp"

namespace cse::help
{
  struct window_previous
  {
    window_previous();
    window_previous(const window_state &state_, const window_graphics &graphics_);
    ~window_previous() = default;
    window_previous(const window_previous &) = delete;
    window_previous &operator=(const window_previous &) = delete;
    window_previous(window_previous &&) = delete;
    window_previous &operator=(window_previous &&) = delete;

    window_state state{};
    window_graphics graphics{};
  };

  struct camera_previous
  {
    camera_previous();
    camera_previous(const camera_state &state_, const camera_graphics &graphics_);
    ~camera_previous() = default;
    camera_previous(const camera_previous &) = delete;
    camera_previous &operator=(const camera_previous &) = delete;
    camera_previous(camera_previous &&) = delete;
    camera_previous &operator=(camera_previous &&) = delete;

    camera_state state{};
    camera_graphics graphics{};
  };

  struct object_previous
  {
    object_previous();
    object_previous(const object_state &state_, const object_graphics &graphics_);
    ~object_previous() = default;
    object_previous(const object_previous &) = delete;
    object_previous &operator=(const object_previous &) = delete;
    object_previous(object_previous &&) = delete;

    object_state state{};
    object_graphics graphics{};
  };
}
