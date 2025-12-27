#pragma once

#include "game.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "glm/ext/vector_uint2.hpp"

#include "id.hpp"
#include "traits.hpp"
#include "window.hpp"

namespace cse
{
  template <typename window_type, typename... window_arguments>
  void game::set_window(const std::string &title, const glm::uvec2 &dimensions, window_arguments &&...arguments)
  {
    window = std::make_shared<window_type>(title, dimensions, std::forward<window_arguments>(arguments)...);
    window->parent = weak_from_this();
  }

  template <typename scene_type, typename... scene_arguments>
  void game::set_scene(const help::id name, const std::function<void(const std::shared_ptr<scene_type>)> &config,
                       scene_arguments &&...arguments)
  {
    auto scene{std::make_shared<scene_type>(std::forward<scene_arguments>(arguments)...)};
    scene->parent = weak_from_this();
    config(scene);
    scenes.emplace(name, scene);

    if (window->running)
      if (auto current{current_scene.lock()})
        current == scene ? scene->initialize(window->graphics.instance, window->graphics.gpu) : void();
  }

  template <typename callable, typename... scene_arguments>
  void game::set_scene(const help::id name, callable &&config, scene_arguments &&...arguments)
  {
    using scene_type = typename help::scene_type_from_callable<callable>::extracted_type;
    set_scene<scene_type, scene_arguments...>(
      name, std::function<void(const std::shared_ptr<scene_type>)>(std::forward<callable>(config)),
      std::forward<scene_arguments>(arguments)...);
  }

  template <typename scene_type, typename... scene_arguments>
  void game::set_current_scene(const help::id name,
                               const std::function<void(const std::shared_ptr<scene_type>)> &config,
                               scene_arguments &&...arguments)
  {
    set_scene<scene_type, scene_arguments...>(name, config, std::forward<scene_arguments>(arguments)...);
    current_scene = scenes.at(name);
  }

  template <typename callable, typename... scene_arguments>
  void game::set_current_scene(const help::id name, callable &&config, scene_arguments &&...arguments)
  {
    using scene_type = typename help::scene_type_from_callable<callable>::extracted_type;
    set_current_scene<scene_type, scene_arguments...>(
      name, std::function<void(const std::shared_ptr<scene_type>)>(std::forward<callable>(config)),
      std::forward<scene_arguments>(arguments)...);
  }
}
