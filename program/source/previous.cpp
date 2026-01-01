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
    graphics.title.change = nullptr;
  }

  window_previous::window_previous(const window_state &state_, const window_graphics &graphics_)
  {
    state.width.change = nullptr;
    state.height.change = nullptr;
    state.left.change = nullptr;
    state.top.change = nullptr;
    state.display_index.change = nullptr;
    state.fullscreen.change = nullptr;
    state.vsync.change = nullptr;
    graphics.title.change = nullptr;

    state.running = state_.running;
    state.width = state_.width;
    state.height = state_.height;
    state.left = state_.left;
    state.top = state_.top;
    state.display_index = state_.display_index;
    state.fullscreen = state_.fullscreen;
    state.vsync = state_.vsync;
    graphics.title = graphics_.title;
  }

  camera_previous::camera_previous() {}

  camera_previous::camera_previous(const camera_state &state_, const camera_graphics &graphics_)
  {
    state.translation = state_.translation;
    state.forward = state_.forward;
    state.up = state_.up;
    graphics.fov = graphics_.fov;
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

    state.translation = state_.translation;
    state.rotation = state_.rotation;
    state.scale = state_.scale;
    graphics.color = graphics_.color;
    graphics.shader.vertex = graphics_.shader.vertex;
    graphics.shader.fragment = graphics_.shader.fragment;
    graphics.texture.image = graphics_.texture.image;
    graphics.texture.group = graphics_.texture.group;
    graphics.texture.animation = graphics_.texture.animation;
  }
}
