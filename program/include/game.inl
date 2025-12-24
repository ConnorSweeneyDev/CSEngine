#pragma once

#include "game.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "glm/ext/vector_uint2.hpp"

#include "id.hpp"

namespace cse
{
  template <typename window_type, typename... window_arguments>
  void game::set_window(const std::string &title, const glm::uvec2 &dimensions, window_arguments &&...arguments)
  {
    window = std::make_shared<window_type>(title, dimensions, std::forward<window_arguments>(arguments)...);
  }

  template <typename scene_type, typename... scene_arguments>
  void game::set_scene(const help::id name, const std::function<void(const std::shared_ptr<scene_type>)> &config,
                       scene_arguments &&...arguments)
  {
    auto scene{std::make_shared<scene_type>(std::forward<scene_arguments>(arguments)...)};
    scenes.emplace(name, scene);
    config(scene);
  }

  template <typename scene_type, typename... scene_arguments>
  void game::set_current_scene(const help::id name,
                               const std::function<void(const std::shared_ptr<scene_type>)> &config,
                               scene_arguments &&...arguments)
  {
    set_scene<scene_type, scene_arguments...>(name, config, std::forward<scene_arguments>(arguments)...);
    current_scene = scenes.at(name);
  }
}
