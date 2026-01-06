#pragma once

#include "game.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "exception.hpp"
#include "name.hpp"
#include "scene.hpp"
#include "traits.hpp"

namespace cse
{
  template <help::is_game game_type, typename... game_arguments> std::shared_ptr<game_type>
  game::create(const std::function<void(const std::shared_ptr<game_type>)> &config, game_arguments &&...arguments)
  {
    if (!instance.expired()) throw exception("Tried to create a second game instance");
    auto new_instance{std::shared_ptr<game_type>{new game_type{std::forward<game_arguments>(arguments)...}}};
    if (config) config(new_instance);
    instance = new_instance;
    return new_instance;
  }

  template <help::is_callable callable, typename... game_arguments>
  std::shared_ptr<game> game::create(callable &&config, game_arguments &&...arguments)
  {
    using game_type = typename help::type_from_callable<callable>::extracted_type;
    return create<game_type, game_arguments...>(
      std::function<void(const std::shared_ptr<game_type>)>(std::forward<callable>(config)),
      std::forward<game_arguments>(arguments)...);
  }

  template <help::is_window window_type, typename... window_arguments>
  std::shared_ptr<game> game::set_window(window_arguments &&...arguments)
  {
    auto window{std::make_shared<window_type>(std::forward<window_arguments>(arguments)...)};
    if (auto parent{weak_from_this()}; !parent.expired()) window->state.active.parent = parent;
    if (state.created)
      state.next.window = window;
    else
    {
      state.active.window = window;
      state.previous.window = window;
    }
    return shared_from_this();
  }

  template <help::is_scene scene_type, typename... scene_arguments>
  std::shared_ptr<game> game::set_scene(const help::name name,
                                        const std::function<void(const std::shared_ptr<scene_type>)> &config,
                                        scene_arguments &&...arguments)
  {
    auto scene{std::make_shared<scene_type>(std::forward<scene_arguments>(arguments)...)};
    if (auto parent{weak_from_this()}; !parent.expired()) scene->state.active.parent = parent;
    if (config) config(scene);
    if (auto iterator{state.active.scenes.find(name)}; state.created && iterator != state.active.scenes.end())
    {
      auto target{iterator->second};
      if (state.active.scene.pointer == target)
      {
        state.next.scene = {name, scene};
        return shared_from_this();
      }
      else
        target->clean();
    }
    state.active.scenes.insert_or_assign(name, scene);
    if (state.created && !scene->state.prepared) scene->prepare();
    return shared_from_this();
  }

  template <help::is_callable callable, typename... scene_arguments>
  std::shared_ptr<game> game::set_scene(const help::name name, callable &&config, scene_arguments &&...arguments)
  {
    using scene_type = typename help::type_from_callable<callable>::extracted_type;
    return set_scene<scene_type, scene_arguments...>(
      name, std::function<void(const std::shared_ptr<scene_type>)>(std::forward<callable>(config)),
      std::forward<scene_arguments>(arguments)...);
  }

  template <help::is_scene scene_type, typename... scene_arguments>
  std::shared_ptr<game> game::set_current_scene(const help::name name,
                                                const std::function<void(const std::shared_ptr<scene_type>)> &config,
                                                scene_arguments &&...arguments)
  {
    auto scene{std::make_shared<scene_type>(std::forward<scene_arguments>(arguments)...)};
    if (auto parent{weak_from_this()}; !parent.expired()) scene->state.active.parent = parent;
    if (config) config(scene);
    if (state.created)
      state.next.scene = {name, scene};
    else
    {
      state.active.scenes.insert_or_assign(name, scene);
      state.active.scene = {name, scene};
      state.previous.scene = {name, scene};
    }
    return shared_from_this();
  }

  template <help::is_callable callable, typename... scene_arguments> std::shared_ptr<game>
  game::set_current_scene(const help::name name, callable &&config, scene_arguments &&...arguments)
  {
    using scene_type = typename help::type_from_callable<callable>::extracted_type;
    return set_current_scene<scene_type, scene_arguments...>(
      name, std::function<void(const std::shared_ptr<scene_type>)>(std::forward<callable>(config)),
      std::forward<scene_arguments>(arguments)...);
  }
}
