#include "previous.hpp"

#include "declaration.hpp"
#include "graphics.hpp"

namespace cse::help
{
  window_previous::window_previous()
  {
    state.width.change = nullptr;
    state.height.change = nullptr;
    state.left.change = nullptr;
    state.top.change = nullptr;
    state.display_index.change = nullptr;
    state.fullscreen.change = nullptr;
    state.vsync.change = nullptr;
  }

  window_previous::window_previous(const window_state &state_)
  {
    state.width.change = nullptr;
    state.height.change = nullptr;
    state.left.change = nullptr;
    state.top.change = nullptr;
    state.display_index.change = nullptr;
    state.fullscreen.change = nullptr;
    state.vsync.change = nullptr;

    state.running = state_.running;
    state.width = state_.width;
    state.height = state_.height;
    state.left = state_.left;
    state.top = state_.top;
    state.display_index = state_.display_index;
    state.fullscreen = state_.fullscreen;
    state.vsync = state_.vsync;
  }

  object_previous::object_previous()
  {
    graphics.texture.image.change = nullptr;
    graphics.shader.fragment.change = nullptr;
    graphics.shader.vertex.change = nullptr;
  }

  object_previous::object_previous(const object_state &state_, const object_graphics &graphics_)
  {
    graphics.texture.image.change = nullptr;
    graphics.shader.fragment.change = nullptr;
    graphics.shader.vertex.change = nullptr;

    state = state_;
    graphics.color = graphics_.color;
    graphics.shader.vertex = graphics_.shader.vertex;
    graphics.shader.fragment = graphics_.shader.fragment;
    graphics.texture.image = graphics_.texture.image;
    graphics.texture.group = graphics_.texture.group;
    graphics.texture.animation = graphics_.texture.animation;
  }
}
