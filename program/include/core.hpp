#pragma once

#include <type_traits>

namespace cse
{
  class game;
  class window;
  class scene;
  class interface;
  class camera;
  class object;
  class light;
}

namespace cse::help
{
  enum class phase
  {
    CLEANED,
    PREPARED,
    CREATED
  };
  namespace game
  {
    struct previous;
    struct active;
    struct next;
  }
  namespace window
  {
    struct previous;
    struct active;
  }
  namespace scene
  {
    struct previous;
    struct active;
    struct next;
  }
  namespace interface
  {
    struct previous;
    struct active;
  }
  namespace camera
  {
    struct previous;
    struct active;
  }
  namespace object
  {
    struct previous;
    struct active;
  }
  namespace light
  {
    struct previous;
    struct active;
  }
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
  concept is_interface = std::is_base_of_v<interface, type>;
  template <typename type>
  concept is_camera = std::is_base_of_v<camera, type>;
  template <typename type>
  concept is_object = std::is_base_of_v<object, type>;
  template <typename type>
  concept is_light = std::is_base_of_v<light, type>;
}
