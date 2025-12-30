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
  template <help::is_window window_type, typename... window_arguments>
  void game::set_window(const std::string &title, const glm::uvec2 &dimensions, window_arguments &&...arguments)
  {
    window = std::make_shared<window_type>(title, dimensions, std::forward<window_arguments>(arguments)...);
    window->parent = weak_from_this();
  }

  template <help::is_scene scene_type, typename... scene_arguments>
  void game::set_scene(const help::id name, const std::function<void(const std::shared_ptr<scene_type>)> &config,
                       scene_arguments &&...arguments)
  {
    auto scene{std::make_shared<scene_type>(std::forward<scene_arguments>(arguments)...)};
    scene->parent = weak_from_this();
    config(scene);
    if (window->state.running)
      if (auto current{current_scene.lock()})
        if (auto iterator{scenes.find(name)}; iterator != scenes.end() && current == iterator->second)
        {
          pending_scene = {name, scene};
          return;
        }
    scenes.insert_or_assign(name, scene);
  }

  template <typename callable, typename... scene_arguments>
  void game::set_scene(const help::id name, callable &&config, scene_arguments &&...arguments)
  {
    using scene_type = typename help::scene_type_from_callable<callable>::extracted_type;
    set_scene<scene_type, scene_arguments...>(
      name, std::function<void(const std::shared_ptr<scene_type>)>(std::forward<callable>(config)),
      std::forward<scene_arguments>(arguments)...);
  }

  template <help::is_scene scene_type, typename... scene_arguments>
  void game::set_current_scene(const help::id name,
                               const std::function<void(const std::shared_ptr<scene_type>)> &config,
                               scene_arguments &&...arguments)
  {
    auto scene{std::make_shared<scene_type>(std::forward<scene_arguments>(arguments)...)};
    scene->parent = weak_from_this();
    config(scene);
    if (window->state.running)
      pending_scene = {name, scene};
    else
    {
      scenes.insert_or_assign(name, scene);
      current_scene = scene;
    }
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
