#pragma once

#include "game.hpp"

#include <string>

#include "exception.hpp"

namespace cse::core
{
  template <typename window_type, typename... window_arguments> void game::set_window(window_arguments &&...arguments)
  {
    window = std::make_unique<window_type>(std::forward<window_arguments>(arguments)...);
  }

  template <typename scene_type, typename... scene_arguments>
  void game::add_scene(const std::string &name, scene_arguments &&...arguments)
  {
    if (scenes.find(name) != scenes.end()) throw utility::exception("Scene with name '{}' already exists", name);
    scenes[name] = std::make_shared<scene_type>(std::forward<scene_arguments>(arguments)...);
  }
}
