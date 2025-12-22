#pragma once

#include "game.hpp"

#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <tuple>

#include "glm/ext/vector_uint2.hpp"

#include "exception.hpp"
#include "id.hpp"

namespace cse::core
{
  template <typename window_type, typename... window_arguments>
  void game::set_window(const std::string &title, const glm::uvec2 &dimensions, window_arguments &&...arguments)
  {
    window = std::make_unique<window_type>(title, dimensions, std::forward<window_arguments>(arguments)...);
  }

  template <typename scene_type, typename... scene_arguments>
  void game::add_scene(const helper::id name, std::function<void(std::shared_ptr<scene_type>)> config,
                       scene_arguments &&...arguments)
  {
    if (scenes.contains(name)) throw utility::exception("Tried to add duplicate scene to game");
    auto scene_shared = std::make_shared<scene_type>(std::forward<scene_arguments>(arguments)...);
    scenes.emplace(name, scene_shared);
    config(scene_shared);
  }

  template <typename scene_type, typename... scene_arguments> void game::add_scenes(
    std::initializer_list<std::tuple<helper::id, std::function<void(std::shared_ptr<scene_type>)>, scene_arguments...>>
      new_scenes)
  {
    for (const auto &scene : new_scenes)
      std::apply([this](const auto name, const auto &config, auto &&...arguments)
                 { add_scene<scene_type>(name, config, std::forward<decltype(arguments)>(arguments)...); }, scene);
  }
}
