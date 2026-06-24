#pragma once

#include "game.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include "container.hpp"
#include "core.hpp"
#include "exception.hpp"
#include "function.hpp"
#include "interface.hpp"
#include "name.hpp"
#include "scene.hpp"
#include "state.hpp"

namespace cse
{
  template <trait::is_game game_type, typename... game_arguments> std::shared_ptr<game_type>
  game::create(const std::function<void(const std::shared_ptr<game_type> &)> &config, game_arguments &&...arguments)
  {
    if (!instance.expired()) throw exception("Tried to create a second game instance");
    auto new_instance{std::shared_ptr<game_type>{new game_type{std::forward<game_arguments>(arguments)...}}};
    if (config) config(new_instance);
    instance = new_instance;
    return new_instance;
  }

  template <trait::is_callable callable, typename... game_arguments>
  std::shared_ptr<game> game::create(callable &&config, game_arguments &&...arguments)
  {
    using game_type = typename trait::callable_smart_inner<callable>::type;
    return create<game_type, game_arguments...>(
      std::function<void(const std::shared_ptr<game_type>)>(std::forward<callable>(config)),
      std::forward<game_arguments>(arguments)...);
  }

  template <trait::is_window window_type, typename... window_arguments> game &game::set(window_arguments &&...arguments)
  {
    auto window{std::make_shared<window_type>(std::forward<window_arguments>(arguments)...)};
    window->game = this;
    if (state.active.phase == help::phase::CREATED)
      state.next.window = window;
    else
    {
      state.active.window = window;
      state.previous.window = window;
    }
    return *this;
  }

  template <trait::is_scene scene_type, typename... scene_arguments>
  game &game::set(const name scene_name, const std::function<void(const std::shared_ptr<scene_type> &)> &config,
                  scene_arguments &&...arguments)
  {
    auto scene{std::make_shared<scene_type>(std::forward<scene_arguments>(arguments)...)};
    scene->name = scene_name;
    scene->game = this;
    if (config) config(scene);
    if (auto target{try_find(state.active.scenes, scene_name)}; state.active.phase == help::phase::CREATED && target)
    {
      if (state.active.scene == target)
      {
        state.next.scene = {scene_name, scene};
        return *this;
      }
      else
        target->clean();
    }
    set_or_add(state.active.scenes, scene);
    if (state.active.phase == help::phase::CREATED) scene->prepare();
    return *this;
  }

  template <trait::is_callable callable, typename... scene_arguments>
  game &game::set(const name scene_name, callable &&config, scene_arguments &&...arguments)
  {
    using scene_type = typename trait::callable_smart_inner<callable>::type;
    return set<scene_type, scene_arguments...>(
      scene_name, std::function<void(const std::shared_ptr<scene_type>)>(std::forward<callable>(config)),
      std::forward<scene_arguments>(arguments)...);
  }

  template <trait::is_scene scene_type, typename... scene_arguments>
  game &game::current(const name scene_name, const std::function<void(const std::shared_ptr<scene_type> &)> &config,
                      scene_arguments &&...arguments)
  {
    auto scene{std::make_shared<scene_type>(std::forward<scene_arguments>(arguments)...)};
    scene->name = scene_name;
    scene->game = this;
    if (config) config(scene);
    if (state.active.phase == help::phase::CREATED)
      state.next.scene = {scene_name, scene};
    else
    {
      set_or_add(state.active.scenes, scene);
      state.active.scene = scene;
      state.previous.scene = scene;
    }
    return *this;
  }

  template <trait::is_callable callable, typename... scene_arguments>
  game &game::current(const name scene_name, callable &&config, scene_arguments &&...arguments)
  {
    using scene_type = typename trait::callable_smart_inner<callable>::type;
    return current<scene_type, scene_arguments...>(
      scene_name, std::function<void(const std::shared_ptr<scene_type>)>(std::forward<callable>(config)),
      std::forward<scene_arguments>(arguments)...);
  }

  template <trait::is_interface interface_type, typename... interface_arguments>
  game &game::set(const name interface_name, interface_arguments &&...arguments)
  {
    auto interface{std::make_shared<interface_type>(std::forward<interface_arguments>(arguments)...)};
    interface->name = interface_name;
    interface->game = this;
    interface->scene = std::nullopt;
    switch (state.active.phase)
    {
      case help::phase::CLEANED: set_or_add(state.active.interfaces, interface); break;
      case help::phase::PREPARED:
        if (auto existing{try_find(state.active.interfaces, interface_name)}) existing->clean();
        set_or_add(state.active.interfaces, interface);
        interface->prepare();
        break;
      case help::phase::CREATED:
        if (try_contains(state.active.interfaces, interface_name)) state.interface_removals.insert(interface_name);
        set_or_add(state.interface_additions, interface);
        break;
    }
    return *this;
  }

  template <typename target_type>
    requires(std::is_void_v<target_type> || trait::is_scene<target_type> || trait::is_interface<target_type>)
  game &game::remove(const name target_name)
  {
    if constexpr (std::is_void_v<target_type> || trait::is_scene<target_type>)
      if (auto iterator{try_iterate(state.active.scenes, target_name)}; iterator != state.active.scenes.end())
      {
        const auto &scene{*iterator};
        if (state.active.scene == scene || scene->state.active.phase == help::phase::CREATED)
          throw exception("Tried to remove current or created scene '{}'", target_name.string());
        scene->clean();
        state.active.scenes.erase(iterator);
        return *this;
      }
    if constexpr (std::is_void_v<target_type> || trait::is_interface<target_type>)
      if (auto iterator{try_iterate(state.active.interfaces, target_name)}; iterator != state.active.interfaces.end())
      {
        if (auto &interface{*iterator}; state.active.phase == help::phase::CREATED)
          state.interface_removals.insert(target_name);
        else
        {
          if (interface->state.active.phase == help::phase::PREPARED) interface->clean();
          state.active.interfaces.erase(iterator);
        }
      }
    return *this;
  }
}
