#pragma once

#include "game.hpp"

#include <memory>
#include <string>

#include "exception.hpp"
#include "id.hpp"
#include "scene.hpp"

namespace cse::core
{
  template <typename window_type, typename... window_arguments> void game::set_window(window_arguments &&...arguments)
  {
    window = std::make_unique<window_type>(std::forward<window_arguments>(arguments)...);
  }

  template <typename scene_type, typename... scene_arguments>
  std::weak_ptr<scene> game::add_scene(const helper::id name, scene_arguments &&...arguments)
  {
    if (scenes.contains(name)) throw utility::exception("Tried to add duplicate scene to game");
    scenes.emplace(name, std::make_shared<scene_type>(std::forward<scene_arguments>(arguments)...));
    return get_scene(name);
  }
}
