#include "game.hpp"

#include <algorithm>
#include <memory>
#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_timer.h"

#include "exception.hpp"
#include "id.hpp"
#include "print.hpp"
#include "scene.hpp"
#include "system.hpp"
#include "utility.hpp"
#include "window.hpp"

namespace cse
{
  game::~game()
  {
    instance.reset();
    hook.reset();
    pending_scene.reset();
    current_scene.reset();
    scenes.clear();
    window.reset();
  }

  void game::set_current_scene(const help::id name)
  {
    if (auto iterator{scenes.find(name)}; iterator == scenes.end())
      throw exception("Tried to set current scene to null");
    else if (window->initialized)
      pending_scene = {name, {}};
    else
      current_scene = iterator->second;
  }

  void game::remove_scene(const help::id name)
  {
    if (auto iterator{scenes.find(name)}; iterator != scenes.end())
    {
      const auto &scene{iterator->second};
      if (auto current{current_scene.lock()}; current == scene) throw exception("Tried to remove current scene");
      if (scene->initialized) scene->cleanup(window->graphics.gpu);
      scenes.erase(iterator);
    }
  }

  void game::run()
  {
    initialize();
    while (window->state.running)
    {
      update_time();
      while (simulation_behind())
      {
        process_updates();
        event();
        input();
        simulate();
        update_previous();
      }
      if (should_render())
      {
        render();
        update_fps();
      }
    }
    cleanup();
  }

  void game::initialize()
  {
    update_parents();
    hook.call<void()>("pre_initialize");
    if (!window) throw exception("No window has been set for the game");
    if (!window->initialized) window->initialize();
    if (scenes.empty()) throw exception("No scenes have been added to the game");
    if (auto scene{current_scene.lock()})
    {
      if (!scene->initialized) scene->initialize(window->graphics.instance, window->graphics.gpu);
    }
    else
      throw exception("No current scene has been set for the game");
    hook.call<void()>("post_initialize");
  }

  void game::event()
  {
    if (auto scene{current_scene.lock()})
      while (SDL_PollEvent(&window->current_event))
      {
        hook.call<void(const SDL_Event &)>("pre_event", window->current_event);
        if (!window->initialized) throw exception("Window is not initialized");
        window->event();
        if (!scene->initialized) throw exception("Current scene is not initialized");
        scene->event(window->current_event);
        hook.call<void(const SDL_Event &)>("post_event", window->current_event);
      }
    else
      throw exception("Current scene is null");
  }

  void game::input()
  {
    window->current_keys = SDL_GetKeyboardState(nullptr);
    hook.call<void(const bool *)>("pre_input", window->current_keys);
    window->input();
    if (auto scene{current_scene.lock()})
      scene->input(window->current_keys);
    else
      throw exception("Current scene is not initialized");
    hook.call<void(const bool *)>("post_input", window->current_keys);
  }

  void game::simulate()
  {
    hook.call<void(const float)>("pre_simulate", static_cast<float>(active_poll_rate));
    window->simulate(active_poll_rate);
    if (auto scene{current_scene.lock()})
      scene->simulate(active_poll_rate);
    else
      throw exception("Current scene is not initialized");
    hook.call<void(const float)>("post_simulate", static_cast<float>(active_poll_rate));
  }

  void game::render()
  {
    hook.call<void()>("pre_render");
    if (!window->start_render(aspect_ratio)) return;
    if (auto scene{current_scene.lock()})
      scene->render(window->graphics.gpu, window->graphics.command_buffer, window->graphics.render_pass, alpha,
                    aspect_ratio, scale_factor);
    else
      throw exception("Current scene is not initialized");
    window->end_render();
    hook.call<void()>("post_render");
  }

  void game::cleanup()
  {
    hook.call<void()>("pre_cleanup");
    if (auto scene{current_scene.lock()})
    {
      if (scene->initialized) scene->cleanup(window->graphics.gpu);
    }
    else
      throw exception("Current scene is not initialized");
    window->cleanup();
    hook.call<void()>("post_cleanup");
  }

  void game::update_parents()
  {
    if (window && window->parent.expired()) window->parent = weak_from_this();
    for (const auto &[name, scene] : scenes)
      if (scene->parent.expired()) scene->parent = weak_from_this();
  }

  void game::process_updates()
  {
    if (pending_scene.has_value())
    {
      auto current{current_scene.lock()};
      if (!current) throw exception("Current scene is null");
      if (auto &[name, scene]{pending_scene.value()}; !scene)
      {
        if (auto iterator{scenes.find(name)}; iterator == scenes.end())
          throw exception("Tried to set current scene to null");
        else
        {
          const auto &new_scene{iterator->second};
          if (current->initialized) current->cleanup(window->graphics.gpu);
          current_scene = new_scene;
          if (!new_scene->initialized) new_scene->initialize(window->graphics.instance, window->graphics.gpu);
        }
      }
      else
      {
        if (current->initialized) current->cleanup(window->graphics.gpu);
        scenes.insert_or_assign(name, scene);
        current_scene = scene;
        if (!scene->initialized) scene->initialize(window->graphics.instance, window->graphics.gpu);
      }
      pending_scene.reset();
    }
    if (auto current{current_scene.lock()}; current && current->initialized)
      current->process_updates();
    else
      throw exception("Current scene is not initialized");
  }

  void game::update_previous()
  {
    window->previous.update(window->state, window->graphics);
    auto current{current_scene.lock()};
    if (!current) throw exception("Current scene is null");
    previous_scene = {[this, &current]
                      {
                        for (const auto &[name, scene] : scenes)
                          if (scene == current) return name;
                        return help::id{};
                      }(),
                      current};
    current->update_previous();
  }

  void game::update_time()
  {
    constexpr double minimum_poll_rate{10.0};
    constexpr double minimum_frame_rate{1.0};
    poll_rate = std::max(poll_rate, minimum_poll_rate);
    frame_rate = std::max(frame_rate, minimum_frame_rate);
    const double real_poll_rate = 1.0 / poll_rate;
    const double real_frame_rate = 1.0 / frame_rate;
    if (!equal(real_poll_rate, active_poll_rate))
    {
      accumulator = accumulator * (real_poll_rate / active_poll_rate);
      active_poll_rate = real_poll_rate;
    }
    if (!equal(real_frame_rate, active_frame_rate)) active_frame_rate = real_frame_rate;
    time = static_cast<double>(SDL_GetTicksNS()) / 1e9;
    static double simulation_time{};
    double delta_time{time - simulation_time};
    simulation_time = time;
    if (delta_time > 0.1) delta_time = 0.1;
    accumulator += delta_time;
  }

  bool game::simulation_behind()
  {
    if (accumulator >= active_poll_rate)
    {
      accumulator -= active_poll_rate;
      return true;
    }
    return false;
  }

  bool game::should_render()
  {
    static double render_time{};
    if (time - render_time >= active_frame_rate)
    {
      render_time = time;
      alpha = accumulator / active_poll_rate;
      return true;
    }
    return false;
  }

  void game::update_fps()
  {
    static double fps_time{};
    static unsigned int frame_count{};
    frame_count++;
    if (time - fps_time >= 1.0)
    {
      if constexpr (debug) print<CLOG>("{} FPS\n", frame_count);
      fps_time = time;
      frame_count = 0;
    }
  }
}
