#include "game.hpp"

#include <memory>
#include <string>

#include "SDL3/SDL_log.h"
#include "SDL3/SDL_timer.h"

#include "exception.hpp"
#include "scene.hpp"
#include "window.hpp"

namespace cse::core
{
  game::game() {}

  game::~game()
  {
    current_scene.reset();
    scenes.clear();
    window.reset();
  }

  void game::run()
  {
    initialize();
    while (window->running)
    {
      update_simulation_time();
      while (simulation_behind())
      {
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

  void game::set_current_scene(const std::string &name)
  {
    if (scenes.find(name) == scenes.end()) throw utility::exception("Scene with name '{}' does not exist", name);
    current_scene = scenes[name];
  }

  std::weak_ptr<scene> game::get_scene(const std::string &name) const
  {
    auto iterator = scenes.find(name);
    if (iterator == scenes.end()) throw utility::exception("Scene with name '{}' does not exist", name);
    if (iterator->second == nullptr) throw utility::exception("Scene with name '{}' is not initialized", name);
    return iterator->second;
  }

  void game::initialize()
  {
    window->initialize();
    if (scenes.empty()) throw cse::utility::exception("No scenes have been added to the game");
    if (current_scene.expired()) throw cse::utility::exception("No current scene has been set for the game");
    if (auto scene = current_scene.lock())
      scene->initialize(window->graphics.instance, window->graphics.gpu);
    else
      throw cse::utility::exception("Current scene is not initialized");
  }

  void game::cleanup()
  {
    if (auto scene = current_scene.lock())
      scene->cleanup(window->graphics.gpu);
    else
      throw cse::utility::exception("Current scene is not initialized");
    window->cleanup();
  }

  void game::input()
  {
    window->input();
    if (auto scene = current_scene.lock())
      scene->input(window->keys);
    else
      throw cse::utility::exception("Current scene is not initialized");
  }

  void game::simulate()
  {
    if (auto scene = current_scene.lock())
      scene->simulate(simulation_alpha);
    else
      throw cse::utility::exception("Current scene is not initialized");
  }

  void game::render()
  {
    if (!window->start_render()) return;
    if (auto scene = current_scene.lock())
      scene->render(window->graphics.gpu, window->graphics.command_buffer, window->graphics.render_pass,
                    window->graphics.width, window->graphics.height, scale_factor);
    else
      throw cse::utility::exception("Current scene is not initialized");
    window->end_render();
  }

  void game::update_simulation_time()
  {
    double current_simulation_time = static_cast<double>(SDL_GetTicksNS()) / 1e9;
    double delta_simulation_time = current_simulation_time - last_simulation_time;
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
    double current_render_time = static_cast<double>(SDL_GetTicksNS()) / 1e9;
    if (current_render_time - last_render_time >= target_render_time)
    {
      last_render_time = current_render_time;
      return true;
    }
    return false;
  }

  void game::update_fps()
  {
    frame_count++;
    double current_fps_time = static_cast<double>(SDL_GetTicksNS()) / 1e9;
    if (current_fps_time - last_fps_time >= 1.0)
    {
      SDL_Log("%d FPS", frame_count);
      last_fps_time = current_fps_time;
      frame_count = 0;
    }
  }
}
