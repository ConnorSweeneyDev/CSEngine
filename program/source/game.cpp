#include "game.hpp"

#include <memory>
#include <string>
#include <utility>

#include "SDL3/SDL_log.h"
#include "SDL3/SDL_timer.h"

#include "exception.hpp"
#include "scene.hpp"
#include "window.hpp"

namespace cse::base
{
  game::game(std::unique_ptr<base::window> custom_window) : window(std::move(custom_window))
  {
    if (!window) throw utility::exception("Game window cannot be null");
  }

  game::~game()
  {
    current_scene = nullptr;
    scenes.clear();
    window.reset();
  }

  void game::add_scene(const std::string &name, std::unique_ptr<scene> custom_scene)
  {
    if (!custom_scene) throw utility::exception("Cannot add a null scene with name '{}'", name);
    if (scenes.find(name) != scenes.end()) throw utility::exception("Scene with name '{}' already exists", name);
    scenes[name] = std::move(custom_scene);
  }

  void game::set_current_scene(const std::string &name)
  {
    if (scenes.find(name) == scenes.end()) throw utility::exception("Scene with name '{}' does not exist", name);
    current_scene = scenes[name].get();
  }

  void game::run()
  {
    initialize();
    while (is_running())
    {
      update_simulation_time();
      while (simulation_behind())
      {
        input();
        simulate();
      }
      update_simulation_alpha();
      if (render_behind())
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
    if (!current_scene) throw cse::utility::exception("No current scene has been set for the game");
    current_scene->initialize(window->instance, window->gpu);
  }

  void game::cleanup()
  {
    window->cleanup();
    current_scene->cleanup(window->gpu);
  }

  bool game::is_running() { return window->running; }

  void game::input()
  {
    window->input();
    current_scene->input(window->key_state);
  }

  void game::simulate() { current_scene->simulate(simulation_alpha); }

  void game::render()
  {
    if (!window->start_render()) return;
    current_scene->render(window->command_buffer, window->render_pass, window->width, window->height);
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

  bool game::render_behind()
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
