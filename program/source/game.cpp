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
    current_scene.reset();
    scenes.clear();
    window.reset();
  }

  void game::set_current_scene(const help::id name)
  {
    if (auto iterator{scenes.find(name)}; iterator != scenes.end())
    {
      const auto &scene{iterator->second};
      if (window->state.running)
      {
        if (auto current{current_scene.lock()})
          if (current->initialized) current->cleanup(window->graphics.gpu);
        if (!scene->initialized) scene->initialize(window->graphics.instance, window->graphics.gpu);
      }
      current_scene = scene;
    }
    else
      throw exception("Tried to set current scene to null");
  }

  bool game::remove_scene(const help::id name)
  {
    if (auto iterator{scenes.find(name)}; iterator != scenes.end())
    {
      const auto &scene{iterator->second};
      if (auto current{current_scene.lock()}; current == scene) throw exception("Tried to remove current scene");
      if (window->state.running && scene->initialized) scene->cleanup(window->graphics.gpu);
      scenes.erase(iterator);
      return true;
    }
    return false;
  }

  std::shared_ptr<game> game::create()
  {
    if (!instance.expired()) throw exception("Tried to create a second game instance");
    auto new_instance{std::shared_ptr<game>{new game{}}};
    instance = new_instance;
    return new_instance;
  }

  void game::run()
  {
    initialize();
    while (window->state.running)
    {
      update_simulation_time();
      while (simulation_behind())
      {
        event();
        input();
        simulate();
      }
      update_simulation_alpha();
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
    window->initialize();
    if (scenes.empty()) throw exception("No scenes have been added to the game");
    if (current_scene.expired()) throw exception("No current scene has been set for the game");
    if (auto scene{current_scene.lock()})
    {
      if (!scene->initialized) scene->initialize(window->graphics.instance, window->graphics.gpu);
    }
    else
      throw exception("Current scene is not initialized");
  }

  void game::event()
  {
    while (SDL_PollEvent(&window->current_event))
    {
      window->event();
      if (auto scene{current_scene.lock()})
        scene->event(window->current_event);
      else
        throw exception("Current scene is not initialized");
    }
  }

  void game::input()
  {
    window->input();
    if (auto scene{current_scene.lock()})
      scene->input(window->current_keys);
    else
      throw exception("Current scene is not initialized");
  }

  void game::simulate()
  {
    window->simulate();
    if (auto scene{current_scene.lock()})
      scene->simulate(simulation_alpha);
    else
      throw exception("Current scene is not initialized");
  }

  void game::render()
  {
    if (!window->start_render(target_aspect_ratio)) return;
    if (auto scene{current_scene.lock()})
      scene->render(window->graphics.gpu, window->graphics.command_buffer, window->graphics.render_pass,
                    target_aspect_ratio, global_scale_factor);
    else
      throw exception("Current scene is not initialized");
    window->end_render();
  }

  void game::cleanup()
  {
    if (auto scene{current_scene.lock()})
    {
      if (scene->initialized) scene->cleanup(window->graphics.gpu);
    }
    else
      throw exception("Current scene is not initialized");
    window->cleanup();
  }

  void game::update_simulation_time()
  {
    double current_simulation_time{static_cast<double>(SDL_GetTicksNS()) / 1e9};
    double delta_simulation_time{current_simulation_time - last_simulation_time};
    last_simulation_time = current_simulation_time;
    if (delta_simulation_time > 0.1) delta_simulation_time = 0.1;
    simulation_accumulator += delta_simulation_time;
  }

  bool game::simulation_behind()
  {
    if (simulation_accumulator >= target_simulation_time)
    {
      simulation_accumulator -= target_simulation_time;
      return true;
    }
    return false;
  }

  void game::update_simulation_alpha() { simulation_alpha = simulation_accumulator / target_simulation_time; }

  bool game::should_render()
  {
    double current_render_time{static_cast<double>(SDL_GetTicksNS()) / 1e9};
    if (current_render_time - last_render_time >= target_render_time)
    {
      last_render_time = current_render_time;
      return true;
    }
    return false;
  }

  void game::update_fps()
  {
    current_period_frame_count++;
    double current_fps_time{static_cast<double>(SDL_GetTicksNS()) / 1e9};
    if (current_fps_time - last_fps_time >= 1.0)
    {
      if constexpr (debug) print<CLOG>("{} FPS\n", current_period_frame_count);
      last_fps_time = current_fps_time;
      current_period_frame_count = 0;
    }
  }
}
