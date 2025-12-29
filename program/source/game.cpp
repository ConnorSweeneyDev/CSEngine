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
      scene->simulate(time);
    else
      throw exception("Current scene is not initialized");
  }

  void game::render()
  {
    if (!window->start_render(aspect_ratio)) return;
    if (auto scene{current_scene.lock()})
      scene->render(window->graphics.gpu, window->graphics.command_buffer, window->graphics.render_pass, alpha,
                    aspect_ratio, scale_factor);
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
      alpha = accumulator / frame_rate;
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
