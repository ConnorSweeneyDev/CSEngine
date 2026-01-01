#pragma once

#include "game.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "glm/ext/vector_uint2.hpp"

#include "exception.hpp"
#include "id.hpp"
#include "traits.hpp"
#include "window.hpp"

namespace cse
{
  template <help::is_window window_type, typename... window_arguments>
  void game::set_window(const std::string &title, const glm::uvec2 &dimensions, window_arguments &&...arguments)
  {
    if (window && window->initialized) throw exception("Tried to change window after initialization");
    window = std::make_shared<window_type>(title, dimensions, std::forward<window_arguments>(arguments)...);
    if (auto parent{weak_from_this()}; !parent.expired()) window->parent = parent;
  }

  template <help::is_scene scene_type, typename... scene_arguments>
  void game::set_scene(const help::id name, const std::function<void(const std::shared_ptr<scene_type>)> &config,
                       scene_arguments &&...arguments)
  {
    auto scene{std::make_shared<scene_type>(std::forward<scene_arguments>(arguments)...)};
    if (auto parent{weak_from_this()}; !parent.expired()) scene->parent = parent;
    config(scene);
    if (window && window->initialized)
      if (auto current{state.scene.lock()})
        if (auto iterator{scenes.find(name)}; iterator != scenes.end() && current == iterator->second)
        {
          state.next_scene = {name, scene};
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
    if (auto parent{weak_from_this()}; !parent.expired()) scene->parent = parent;
    config(scene);
    if (window && window->initialized)
      state.next_scene = {name, scene};
    else
    {
      scenes.insert_or_assign(name, scene);
      state.scene = scene;
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

  template <help::is_game game_type, typename... game_arguments>
  std::shared_ptr<game_type> game::create(const std::pair<double, double> &rates, game_arguments &&...arguments)
  {
    if (!instance.expired()) throw exception("Tried to create a second game instance");
    auto new_instance{std::shared_ptr<game_type>{new game_type{rates, std::forward<game_arguments>(arguments)...}}};
    instance = new_instance;
    return new_instance;
  }
}
