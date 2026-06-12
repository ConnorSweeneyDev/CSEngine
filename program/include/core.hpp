#pragma once

#include <type_traits>

namespace cse
{
  class game;
  class window;
  class scene;
  class camera;
  class object;
  class interface;
}

namespace cse::help
{
  struct game_state;
  struct game_graphics;
  struct window_state;
  struct window_graphics;
  struct scene_state;
  struct scene_graphics;
  struct camera_state;
  struct camera_graphics;
  struct object_state;
  struct object_graphics;
  struct interface_state;
  struct interface_graphics;
}

namespace cse::trait
{
  template <typename type>
  concept is_game = std::is_base_of_v<game, type>;
  template <typename type>
  concept is_window = std::is_base_of_v<window, type>;
  template <typename type>
  concept is_scene = std::is_base_of_v<scene, type>;
  template <typename type>
  concept is_camera = std::is_base_of_v<camera, type>;
  template <typename type>
  concept is_object = std::is_base_of_v<object, type>;
  template <typename type>
  concept is_interface = std::is_base_of_v<interface, type>;
}
