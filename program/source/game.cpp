#include "game.hpp"

#include <memory>
#include <string>
#include <utility>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_timer.h"

#include "exception.hpp"
#include "id.hpp"
#include "print.hpp"
#include "scene.hpp"
#include "system.hpp"
#include "window.hpp"

namespace cse::core
{
  game::~game()
  {
    current_scene.reset();
    scenes.clear();
    window.reset();
  }

  std::shared_ptr<window> game::get_window() const noexcept { return window; }

  std::shared_ptr<window> game::get_window_strict() const
  {
    if (window) return window;
    throw utility::exception("Window is not initialized");
  }

  std::shared_ptr<scene> game::get_scene(const helper::id name) const noexcept
  {
    if (!scenes.contains(name)) return nullptr;
    return scenes.at(name);
  }

  std::shared_ptr<scene> game::get_scene_strict(const helper::id name) const
  {
    if (!scenes.contains(name)) throw utility::exception("Requested scene does not exist");
    if (auto scene{scenes.at(name)}) return scene;
    throw utility::exception("Requested scene is not initialized");
  }

  std::pair<helper::id, std::shared_ptr<scene>> game::get_current_scene() const noexcept
  {
    if (current_scene.expired()) return {"", nullptr};
    if (auto scene{current_scene.lock()})
    {
      for (const auto &pair : scenes)
        if (pair.second == scene) return {pair.first, scene};
      return {"", nullptr};
    }
    return {"", nullptr};
  }

  std::pair<helper::id, std::shared_ptr<scene>> game::get_current_scene_strict() const
  {
    if (current_scene.expired()) throw utility::exception("Current scene is not set");
    if (auto scene{current_scene.lock()})
    {
      for (const auto &pair : scenes)
        if (pair.second == scene) return {pair.first, scene};
      throw utility::exception("Current scene is registered in scenes map");
    }
    throw utility::exception("Current scene is not initialized");
  }

  void game::set_current_scene(const helper::id name)
  {
    if (auto scene{get_scene_strict(name)})
      current_scene = scene;
    else
      throw utility::exception("Tried to set current scene to null");
  }

  void game::run()
  {
    initialize();
    while (window->running)
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
    if (scenes.empty()) throw cse::utility::exception("No scenes have been added to the game");
    if (current_scene.expired()) throw cse::utility::exception("No current scene has been set for the game");
    if (auto scene{current_scene.lock()})
      scene->initialize(window->graphics.instance, window->graphics.gpu);
    else
      throw cse::utility::exception("Current scene is not initialized");
  }

  void game::event()
  {
    while (SDL_PollEvent(&window->current_event))
    {
      window->event();
      if (auto scene{current_scene.lock()})
        scene->event(window->current_event);
      else
        throw cse::utility::exception("Current scene is not initialized");
    }
  }

  void game::input()
  {
    window->input();
    if (auto scene{current_scene.lock()})
      scene->input(window->current_keys);
    else
      throw cse::utility::exception("Current scene is not initialized");
  }

  void game::simulate()
  {
    window->simulate();
    if (auto scene{current_scene.lock()})
      scene->simulate(simulation_alpha);
    else
      throw cse::utility::exception("Current scene is not initialized");
  }

  void game::render()
  {
    if (!window->start_render(target_aspect_ratio)) return;
    if (auto scene{current_scene.lock()})
      scene->render(window->graphics.gpu, window->graphics.command_buffer, window->graphics.render_pass,
                    target_aspect_ratio, global_scale_factor);
    else
      throw cse::utility::exception("Current scene is not initialized");
    window->end_render();
  }

  void game::cleanup()
  {
    if (auto scene{current_scene.lock()})
      scene->cleanup(window->graphics.gpu);
    else
      throw cse::utility::exception("Current scene is not initialized");
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
      if constexpr (cse::system::debug) utility::print<CLOG>("{} FPS\n", current_period_frame_count);
      last_fps_time = current_fps_time;
      current_period_frame_count = 0;
    }
  }
}
