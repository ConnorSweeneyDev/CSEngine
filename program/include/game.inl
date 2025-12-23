#pragma once

#include "game.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "glm/ext/vector_uint2.hpp"

#include "exception.hpp"
#include "id.hpp"

namespace cse::core
{
  template <typename window_type> std::shared_ptr<window_type> game::get_window() const noexcept
  {
    return std::static_pointer_cast<window_type>(get_window());
  }

  template <typename window_type> std::shared_ptr<window_type> game::get_window_strict() const
  {
    return std::static_pointer_cast<window_type>(get_window_strict());
  }

  template <typename window_type, typename... window_arguments>
  void game::set_window(const std::string &title, const glm::uvec2 &dimensions, window_arguments &&...arguments)
  {
    window = std::make_shared<window_type>(title, dimensions, std::forward<window_arguments>(arguments)...);
  }

  template <typename scene_type> std::shared_ptr<scene_type> game::get_scene(const helper::id name) const noexcept
  {
    return std::static_pointer_cast<scene_type>(get_scene(name));
  }

  template <typename scene_type> std::shared_ptr<scene_type> game::get_scene_strict(const helper::id name) const
  {
    return std::static_pointer_cast<scene_type>(get_scene_strict(name));
  }

  template <typename scene_type, typename... scene_arguments>
  void game::add_scene(const helper::id name, const std::function<void(const std::shared_ptr<scene_type>)> &config,
                       scene_arguments &&...arguments)
  {
    if (scenes.contains(name)) throw utility::exception("Tried to add duplicate scene to game");
    auto scene{std::make_shared<scene_type>(std::forward<scene_arguments>(arguments)...)};
    scenes.emplace(name, scene);
    config(scene);
  }

  template <typename scene_type>
  std::pair<helper::id, std::shared_ptr<scene_type>> game::get_current_scene() const noexcept
  {
    auto [name, scene]{get_current_scene()};
    return {name, std::static_pointer_cast<scene_type>(scene)};
  }

  template <typename scene_type>
  std::pair<helper::id, std::shared_ptr<scene_type>> game::get_current_scene_strict() const
  {
    auto [name, scene]{get_current_scene_strict()};
    return {name, std::static_pointer_cast<scene_type>(scene)};
  }
}
