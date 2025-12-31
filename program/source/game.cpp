#include "game.hpp"

#include <memory>
#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_timer.h"

#include "exception.hpp"
#include "id.hpp"
#include "print.hpp"
#include "scene.hpp"
#include "system.hpp"
#include "window.hpp"

namespace cse
{
  game::~game()
  {
    pending_scene.reset();
    hook.reset();
    current_scene.reset();
    scenes.clear();
    window.reset();
  }

  void game::set_current_scene(const help::id name)
  {
    if (auto iterator{scenes.find(name)}; iterator == scenes.end())
      throw exception("Tried to set current scene to null");
    else if (window->state.running)
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
      if (window->state.running && scene->initialized) scene->cleanup(window->graphics.gpu);
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
        event();
        input();
        simulate();
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
    window->initialize();
    if (scenes.empty()) throw exception("No scenes have been added to the game");
    if (current_scene.expired()) throw exception("No current scene has been set for the game");
    if (auto scene{current_scene.lock()})
    {
      if (!scene->initialized)
      {
        scene->initialize(window->graphics.instance, window->graphics.gpu);
        process_updates();
      }
    }
    else
      throw exception("Current scene is not initialized");
    hook.call<void()>("post_initialize");
  }

  void game::event()
  {
    while (SDL_PollEvent(&window->current_event))
    {
      hook.call<void()>("pre_event");
      window->event();
      if (auto scene{current_scene.lock()})
        scene->event(window->current_event);
      else
        throw exception("Current scene is not initialized");
      hook.call<void()>("post_event");
    }
  }

  void game::input()
  {
    hook.call<void()>("pre_input");
    window->input();
    if (auto scene{current_scene.lock()})
      scene->input(window->current_keys);
    else
      throw exception("Current scene is not initialized");
    hook.call<void()>("post_input");
  }

  void game::simulate()
  {
    hook.call<void()>("pre_simulate");
    window->simulate();
    if (auto scene{current_scene.lock()})
    {
      scene->simulate(poll_rate);
      process_updates();
    }
    else
      throw exception("Current scene is not initialized");
    hook.call<void()>("post_simulate");
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
    if (!pending_scene.has_value()) return;
    if (auto &[name, scene]{pending_scene.value()}; !scene)
    {
      if (auto iterator{scenes.find(name)}; iterator == scenes.end())
        throw exception("Tried to set current scene to null");
      else
      {
        const auto &new_scene{iterator->second};
        if (auto current{current_scene.lock()})
          if (current->initialized) current->cleanup(window->graphics.gpu);
        if (!new_scene->initialized) new_scene->initialize(window->graphics.instance, window->graphics.gpu);
        current_scene = new_scene;
      }
    }
    else
    {
      if (auto current{current_scene.lock()})
        if (current->initialized) current->cleanup(window->graphics.gpu);
      scenes.insert_or_assign(name, scene);
      if (!scene->initialized) scene->initialize(window->graphics.instance, window->graphics.gpu);
      current_scene = scene;
    }
    pending_scene.reset();
  }

  void game::update_time()
  {
    time = static_cast<double>(SDL_GetTicksNS()) / 1e9;
    static double simulation_time{};
    double delta_time{time - simulation_time};
    simulation_time = time;
    if (delta_time > 0.1) delta_time = 0.1;
    accumulator += delta_time;
  }

  bool game::simulation_behind()
  {
    if (accumulator >= poll_rate)
    {
      accumulator -= poll_rate;
      return true;
    }
    return false;
  }

  bool game::should_render()
  {
    static double render_time{};
    if (time - render_time >= frame_rate)
    {
      render_time = time;
      alpha = accumulator / poll_rate;
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
