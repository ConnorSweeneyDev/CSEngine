#include "previous.hpp"

#include "declaration.hpp"
#include "graphics.hpp"

namespace cse::help
{
  window_previous::window_previous() { clean(); }

  window_previous::window_previous(const window_state &state_, const window_graphics &graphics_)
  {
    clean();
    update(state_, graphics_);
  }

  void window_previous::clean()
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

  void window_previous::update(const window_state &new_state, const window_graphics &new_graphics)
  {
    state.running = new_state.running;
    state.width = new_state.width;
    state.height = new_state.height;
    state.left = new_state.left;
    state.top = new_state.top;
    state.display_index = new_state.display_index;
    state.fullscreen = new_state.fullscreen;
    state.vsync = new_state.vsync;
    graphics.title = new_graphics.title;
  }

  camera_previous::camera_previous() {}

  camera_previous::camera_previous(const camera_state &state_, const camera_graphics &graphics_)
  {
    update(state_, graphics_);
  }

  void camera_previous::update(const camera_state &new_state, const camera_graphics &new_graphics)
  {
    state.translation = new_state.translation;
    state.forward = new_state.forward;
    state.up = new_state.up;
    graphics.fov = new_graphics.fov;
  }

  object_previous::object_previous() { clean(); }

  object_previous::object_previous(const object_state &state_, const object_graphics &graphics_)
  {
    clean();
    update(state_, graphics_);
  }

  void object_previous::clean()
  {
    graphics.texture.image.change = nullptr;
    graphics.shader.fragment.change = nullptr;
    graphics.shader.vertex.change = nullptr;
  }

  void object_previous::update(const object_state &new_state, const object_graphics &new_graphics)
  {
    state.translation = new_state.translation;
    state.rotation = new_state.rotation;
    state.scale = new_state.scale;
    graphics.color = new_graphics.color;
    graphics.shader.vertex = new_graphics.shader.vertex;
    graphics.shader.fragment = new_graphics.shader.fragment;
    graphics.texture.image = new_graphics.texture.image;
    graphics.texture.group = new_graphics.texture.group;
    graphics.texture.animation = new_graphics.texture.animation;
  }
}
