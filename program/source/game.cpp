#include "game.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_timer.h"

#include "exception.hpp"
#include "name.hpp"
#include "print.hpp"
#include "scene.hpp"
#include "system.hpp"
#include "utility.hpp"
#include "window.hpp"

namespace cse
{
  game::game(const double poll_rate_, const double frame_rate, const double aspect_ratio_)
    : state{poll_rate_}, graphics{frame_rate, aspect_ratio_}
  {
  }

  game::~game()
  {
    instance.reset();
    hook.reset();
  }

  std::shared_ptr<game> game::set_current_scene(const help::name name)
  {
    if (auto iterator{state.active.scenes.find(name)}; iterator == state.active.scenes.end())
      throw exception("Tried to set current scene to null");
    else if (state.active.window->state.initialized)
      state.next.scene = {name, {}};
    else
    {
      state.active.scene = {name, iterator->second};
      state.previous.scene = {name, iterator->second};
    }
    return shared_from_this();
  }

  std::shared_ptr<game> game::remove_scene(const help::name name)
  {
    if (auto iterator{state.active.scenes.find(name)}; iterator != state.active.scenes.end())
    {
      const auto &scene{iterator->second};
      if (state.active.scene.pointer == scene) throw exception("Tried to remove current scene");
      if (state.active.scene.pointer->state.initialized) scene->cleanup(state.active.window->graphics.gpu);
      state.active.scenes.erase(iterator);
    }
    return shared_from_this();
  }

  void game::run()
  {
    initialize();
    while (state.active.window->state.active.running)
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
    graphics.initialize_app();
    setup_parents();
    hook.call<void()>("pre_initialize");
    if (!state.active.window) throw exception("No window has been set for the game");
    if (!state.active.window->state.initialized) state.active.window->initialize();
    if (state.active.scenes.empty()) throw exception("No scenes have been added to the game");
    if (!state.active.scene.pointer) throw exception("No current scene has been set for the game");
    if (!state.active.scene.pointer->state.initialized)
      state.active.scene.pointer->initialize(state.active.window->graphics.instance, state.active.window->graphics.gpu);
    hook.call<void()>("post_initialize");
  }

  void game::event()
  {
    while (SDL_PollEvent(&state.active.window->state.event))
    {
      hook.call<void(const SDL_Event &)>("pre_event", state.active.window->state.event);
      if (!state.active.window->state.initialized) throw exception("Window is not initialized");
      state.active.window->event();
      if (!state.active.scene.pointer->state.initialized) throw exception("Current scene is not initialized");
      state.active.scene.pointer->event(state.active.window->state.event);
      hook.call<void(const SDL_Event &)>("post_event", state.active.window->state.event);
    }
  }

  void game::input()
  {
    state.active.window->state.keys = SDL_GetKeyboardState(nullptr);
    hook.call<void(const bool *)>("pre_input", state.active.window->state.keys);
    if (!state.active.window->state.initialized) throw exception("Window is not initialized");
    state.active.window->input();
    if (!state.active.scene.pointer->state.initialized) throw exception("Current scene is not initialized");
    state.active.scene.pointer->input(state.active.window->state.keys);
    hook.call<void(const bool *)>("post_input", state.active.window->state.keys);
  }

  void game::simulate()
  {
    hook.call<void(const float)>("pre_simulate", static_cast<float>(state.actual_poll_rate));
    if (!state.active.window->state.initialized) throw exception("Window is not initialized");
    state.active.window->simulate(state.actual_poll_rate);
    if (!state.active.scene.pointer->state.initialized) throw exception("Current scene is not initialized");
    state.active.scene.pointer->simulate(state.actual_poll_rate);
    hook.call<void(const float)>("post_simulate", static_cast<float>(state.actual_poll_rate));
  }

  void game::render()
  {
    hook.call<void()>("pre_render");
    if (!state.active.window->state.initialized) throw exception("Window is not initialized");
    if (!state.active.window->start_render(graphics.active.aspect_ratio)) return;
    if (!state.active.scene.pointer->state.initialized) throw exception("Current scene is not initialized");
    state.active.scene.pointer->render(state.active.window->graphics.gpu, state.active.window->graphics.command_buffer,
                                       state.active.window->graphics.render_pass, state.alpha,
                                       graphics.active.aspect_ratio);
    state.active.window->end_render();
    hook.call<void()>("post_render");
  }

  void game::cleanup()
  {
    hook.call<void()>("pre_cleanup");
    if (!state.active.scene.pointer->state.initialized) throw exception("Current scene is not initialized");
    state.active.scene.pointer->cleanup(state.active.window->graphics.gpu);
    if (!state.active.window->state.initialized) throw exception("Window is not initialized");
    state.active.window->cleanup();
    graphics.cleanup_app();
    hook.call<void()>("post_cleanup");
  }

  void game::setup_parents()
  {
    if (state.active.window && state.active.window->state.active.parent.expired())
      state.active.window->state.active.parent = weak_from_this();
    for (const auto &[name, scene] : state.active.scenes)
      if (scene->state.active.parent.expired()) scene->state.active.parent = weak_from_this();
  }

  void game::process_updates()
  {
    if (state.next.window.has_value())
    {
      if (auto &window{state.next.window.value()})
      {
        if (state.active.scene.pointer->state.initialized)
          state.active.scene.pointer->cleanup(state.active.window->graphics.gpu);
        if (state.active.window->state.initialized) state.active.window->cleanup();
        state.active.window = window;
        if (!window->state.initialized) window->initialize();
        if (!state.active.scene.pointer->state.initialized)
          state.active.scene.pointer->initialize(state.active.window->graphics.instance,
                                                 state.active.window->graphics.gpu);
      }
      else
        throw exception("Tried to set window to null");
      state.next.window.reset();
    }
    if (state.next.scene.has_value())
    {
      if (auto &[name, scene]{state.next.scene.value()}; !scene)
      {
        if (auto iterator{state.active.scenes.find(name)}; iterator == state.active.scenes.end())
          throw exception("Tried to set current scene to null");
        else if (name != state.active.scene.name)
        {
          const auto &new_scene{iterator->second};
          if (state.active.scene.pointer->state.initialized)
            state.active.scene.pointer->cleanup(state.active.window->graphics.gpu);
          state.active.scene = {name, new_scene};
          if (!new_scene->state.initialized)
            new_scene->initialize(state.active.window->graphics.instance, state.active.window->graphics.gpu);
        }
      }
      else
      {
        if (state.active.scene.pointer->state.initialized)
          state.active.scene.pointer->cleanup(state.active.window->graphics.gpu);
        state.active.scenes.insert_or_assign(name, scene);
        state.active.scene = {name, scene};
        if (!scene->state.initialized)
          scene->initialize(state.active.window->graphics.instance, state.active.window->graphics.gpu);
      }
      state.next.scene.reset();
    }
    if (state.active.scene.pointer->state.initialized) state.active.scene.pointer->process_updates();
  }

  void game::update_previous()
  {
    state.update_previous();
    graphics.update_previous();
    state.active.window->update_previous();
    state.active.scene.pointer->update_previous();
  }

  void game::update_time()
  {
    constexpr double minimum_poll_rate{10.0};
    constexpr double minimum_frame_rate{1.0};
    state.active.poll_rate = std::max(minimum_poll_rate, state.active.poll_rate);
    graphics.active.frame_rate = std::max(minimum_frame_rate, graphics.active.frame_rate);
    const double real_poll_rate = 1.0 / state.active.poll_rate;
    const double real_frame_rate = 1.0 / graphics.active.frame_rate;
    if (!equal(real_poll_rate, state.actual_poll_rate))
    {
      state.accumulator = state.accumulator * (real_poll_rate / state.actual_poll_rate);
      state.actual_poll_rate = real_poll_rate;
    }
    if (!equal(real_frame_rate, graphics.actual_frame_rate)) graphics.actual_frame_rate = real_frame_rate;
    state.time = static_cast<double>(SDL_GetTicksNS()) / 1e9;
    static double simulation_time{};
    double delta_time{state.time - simulation_time};
    simulation_time = state.time;
    if (delta_time > 0.1) delta_time = 0.1;
    state.accumulator += delta_time;
  }

  bool game::simulation_behind()
  {
    if (state.accumulator >= state.actual_poll_rate)
    {
      state.accumulator -= state.actual_poll_rate;
      return true;
    }
    return false;
  }

  bool game::should_render()
  {
    static double render_time{};
    if (state.time - render_time >= graphics.actual_frame_rate)
    {
      render_time = state.time;
      state.alpha = state.accumulator / state.actual_poll_rate;
      return true;
    }
    return false;
  }

  void game::update_fps()
  {
    static double fps_time{};
    static unsigned int frame_count{};
    frame_count++;
    if (state.time - fps_time >= 1.0)
    {
      if constexpr (debug) print<CLOG>("{} FPS\n", frame_count);
      fps_time = state.time;
      frame_count = 0;
    }
  }
}
