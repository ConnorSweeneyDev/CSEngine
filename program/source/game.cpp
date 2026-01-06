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
    else if (state.created)
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
      if (state.active.scene.pointer == scene || scene->state.created)
        throw exception("Tried to remove current or created scene");
      scene->clean();
      state.active.scenes.erase(iterator);
    }
    return shared_from_this();
  }

  void game::run()
  {
    prepare();
    create();
    while (running())
    {
      time();
      while (behind())
      {
        previous();
        sync();
        event();
        input();
        simulate();
      }
      if (ready())
      {
        render();
        fps();
      }
    }
    destroy();
    clean();
  }

  void game::prepare()
  {
    if (state.prepared) throw exception("Game cannot be prepared more than once");
    if (state.created) throw exception("Game cannot be prepared while created");
    if (!state.active.window) throw exception("No window has been set for the game");
    if (state.active.window->state.active.parent.expired()) state.active.window->state.active.parent = weak_from_this();
    for (const auto &[name, scene] : state.active.scenes)
      if (scene->state.active.parent.expired()) scene->state.active.parent = weak_from_this();
    hook.call<void()>("pre_prepare");
    state.active.window->prepare();
    if (state.active.scenes.empty()) throw exception("No scenes have been added to the game");
    if (!state.active.scene.pointer) throw exception("No current scene has been set for the game");
    state.active.scene.pointer->prepare();
    state.prepared = true;
    hook.call<void()>("post_prepare");
  }

  void game::create()
  {
    if (!state.prepared) throw exception("Game must be prepared before creation");
    if (state.created) throw exception("Game cannot be created more than once");
    hook.call<void()>("pre_create");
    graphics.create_app();
    state.active.window->create();
    state.active.scene.pointer->create(state.active.window->graphics.instance, state.active.window->graphics.gpu);
    state.created = true;
    hook.call<void()>("post_create");
  }

  void game::previous()
  {
    if (!state.prepared) throw exception("Game must be prepared before updating previous state");
    if (!state.created) throw exception("Game must be created before updating previous state");
    state.update_previous();
    graphics.update_previous();
    state.active.window->previous();
    state.active.scene.pointer->previous();
  }

  void game::sync()
  {
    if (!state.prepared) throw exception("Game must be prepared before syncing");
    if (!state.created) throw exception("Game must be created before syncing");
    hook.call<void()>("pre_sync");
    if (state.next.window.has_value())
    {
      if (auto &window{state.next.window.value()})
      {
        state.active.scene.pointer->destroy(state.active.window->graphics.gpu);
        state.active.window->destroy();
        state.active.window->clean();
        state.active.window = window;
        window->prepare();
        window->create();
        state.active.scene.pointer->create(state.active.window->graphics.instance, state.active.window->graphics.gpu);
      }
      else
        throw exception("Tried to set window to null");
      state.next.window.reset();
    }
    if (state.next.scene.has_value())
    {
      if (auto &[name, scene]{state.next.scene.value()}; scene)
      {
        state.active.scene.pointer->destroy(state.active.window->graphics.gpu);
        if (name == state.active.scene.name) state.active.scene.pointer->clean();
        state.active.scenes.insert_or_assign(name, scene);
        state.active.scene = {name, scene};
        scene->prepare();
        scene->create(state.active.window->graphics.instance, state.active.window->graphics.gpu);
      }
      else
      {
        if (auto iterator{state.active.scenes.find(name)}; iterator == state.active.scenes.end())
          throw exception("Tried to set current scene to null");
        else if (name != state.active.scene.name)
        {
          const auto &next_scene{iterator->second};
          state.active.scene.pointer->destroy(state.active.window->graphics.gpu);
          state.active.scene = {name, next_scene};
          if (!next_scene->state.prepared) next_scene->prepare();
          next_scene->create(state.active.window->graphics.instance, state.active.window->graphics.gpu);
        }
      }
      state.next.scene.reset();
    }
    state.active.scene.pointer->sync(state.active.window->graphics.instance, state.active.window->graphics.gpu);
    hook.call<void()>("post_sync");
  }

  void game::event()
  {
    if (!state.prepared) throw exception("Game must be prepared before processing events");
    if (!state.created) throw exception("Game must be created before processing events");
    while (SDL_PollEvent(&state.active.window->state.event))
    {
      hook.call<void(const SDL_Event &)>("pre_event", state.active.window->state.event);
      state.active.window->event();
      state.active.scene.pointer->event(state.active.window->state.event);
      hook.call<void(const SDL_Event &)>("post_event", state.active.window->state.event);
    }
  }

  void game::input()
  {
    if (!state.prepared) throw exception("Game must be prepared before processing input");
    if (!state.created) throw exception("Game must be created before processing input");
    state.active.window->state.input = SDL_GetKeyboardState(nullptr);
    hook.call<void(const bool *)>("pre_input", state.active.window->state.input);
    state.active.window->input();
    state.active.scene.pointer->input(state.active.window->state.input);
    hook.call<void(const bool *)>("post_input", state.active.window->state.input);
  }

  void game::simulate()
  {
    if (!state.prepared) throw exception("Game must be prepared before simulation");
    if (!state.created) throw exception("Game must be created before simulation");
    hook.call<void(const float)>("pre_simulate", static_cast<float>(state.actual_poll_rate));
    state.active.window->simulate(static_cast<float>(state.actual_poll_rate));
    state.active.scene.pointer->simulate(static_cast<float>(state.actual_poll_rate));
    hook.call<void(const float)>("post_simulate", static_cast<float>(state.actual_poll_rate));
  }

  void game::render()
  {
    if (!state.prepared) throw exception("Game must be prepared before rendering");
    if (!state.created) throw exception("Game must be created before rendering");
    hook.call<void(const double)>("pre_render", state.alpha);
    if (!state.active.window->pre_render(state.alpha, static_cast<float>(graphics.active.aspect_ratio))) return;
    state.active.scene.pointer->render(state.active.window->graphics.gpu, state.active.window->graphics.command_buffer,
                                       state.active.window->graphics.render_pass, state.alpha,
                                       static_cast<float>(graphics.active.aspect_ratio));
    state.active.window->post_render(state.alpha);
    hook.call<void(const double)>("post_render", state.alpha);
  }

  void game::destroy()
  {
    if (!state.prepared) throw exception("Game must be prepared before destruction");
    if (!state.created) throw exception("Game cannot be destroyed more than once");
    hook.call<void()>("pre_destroy");
    state.active.scene.pointer->destroy(state.active.window->graphics.gpu);
    state.active.window->destroy();
    graphics.destroy_app();
    state.created = false;
    hook.call<void()>("post_destroy");
  }

  void game::clean()
  {
    if (!state.prepared) throw exception("Game cannot be cleaned more than once");
    if (state.created) throw exception("Game must be destroyed before cleaning");
    hook.call<void()>("pre_clean");
    state.active.scene.pointer->clean();
    state.active.window->clean();
    state.prepared = false;
    hook.call<void()>("post_clean");
  }

  bool game::running() { return state.active.window->state.active.running; }

  void game::time()
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

  bool game::behind()
  {
    if (state.accumulator >= state.actual_poll_rate)
    {
      state.accumulator -= state.actual_poll_rate;
      return true;
    }
    return false;
  }

  bool game::ready()
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

  void game::fps()
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
