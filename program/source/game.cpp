#include "game.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_timer.h"
#include "glm/ext/vector_double4.hpp"

#include "exception.hpp"
#include "name.hpp"
#include "numeric.hpp"
#include "print.hpp"
#include "scene.hpp"
#include "state.hpp"
#include "system.hpp"
#include "window.hpp"

namespace cse
{
  game::game(const double tick_, const double frame_, const double aspect_, const glm::dvec4 &clear_)
    : state{tick_}, graphics{frame_, aspect_, clear_}
  {
  }

  game &game::current(const name name)
  {
    if (auto iterator{state.active.scenes.find(name)}; iterator == state.active.scenes.end())
      throw exception("Tried to set current scene to null");
    else if (state.active.phase == help::phase::CREATED)
      state.next.scene = {name, {}};
    else
    {
      state.active.scene = {name, iterator->second};
      state.previous.scene = {name, iterator->second};
    }
    return *this;
  }

  game &game::remove(const name name)
  {
    if (auto iterator{state.active.scenes.find(name)}; iterator != state.active.scenes.end())
    {
      const auto &scene{iterator->second};
      if (state.active.scene.pointer == scene || scene->state.active.phase == help::phase::CREATED)
        throw exception("Tried to remove current or created scene");
      scene->clean();
      state.active.scenes.erase(iterator);
    }
    return *this;
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
        tps();
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
    if (state.active.phase != help::phase::CLEANED) throw exception("Game must be cleaned before preparation");
    if (!state.active.window) throw exception("No window has been set for the game");
    if (state.active.window->state.active.parent.expired()) state.active.window->state.active.parent = weak_from_this();
    for (const auto &[name, scene] : state.active.scenes)
      if (scene->state.active.parent.expired()) scene->state.active.parent = weak_from_this();
    pre_prepare();
    state.active.window->prepare();
    if (state.active.scenes.empty()) throw exception("No scenes have been added to the game");
    if (!state.active.scene.pointer) throw exception("No current scene has been set for the game");
    state.active.scene.pointer->prepare();
    state.active.phase = help::phase::PREPARED;
    post_prepare();
  }

  void game::create()
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Game must be prepared before creation");
    pre_create();
    graphics.create_app();
    state.active.window->create();
    state.active.scene.pointer->create(state.active.window->graphics.instance, state.active.window->graphics.gpu);
    state.active.phase = help::phase::CREATED;
    post_create();
  }

  void game::previous()
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Game must be created before updating previous state");
    pre_previous();
    state.update_previous();
    graphics.update_previous();
    state.active.window->previous();
    state.active.scene.pointer->previous();
    post_previous();
  }

  void game::sync()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Game must be created before synchronization");
    pre_sync();
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
          if (next_scene->state.active.phase == help::phase::CLEANED) next_scene->prepare();
          next_scene->create(state.active.window->graphics.instance, state.active.window->graphics.gpu);
        }
      }
      state.next.scene.reset();
    }
    state.active.scene.pointer->sync(state.active.window->graphics.instance, state.active.window->graphics.gpu);
    post_sync();
  }

  void game::event()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Game must be created before processing events");
    while (SDL_PollEvent(&state.active.window->state.event))
    {
      pre_event(state.active.window->state.event);
      state.active.window->event();
      state.active.scene.pointer->event(state.active.window->state.event);
      post_event(state.active.window->state.event);
    }
  }

  void game::input()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Game must be created before processing input");
    state.active.window->state.input = SDL_GetKeyboardState(nullptr);
    pre_input(state.active.window->state.input);
    state.active.window->input();
    state.active.scene.pointer->input(state.active.window->state.input);
    post_input(state.active.window->state.input);
  }

  void game::simulate()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Game must be created before simulation");
    pre_simulate(state.actual_tick);
    state.active.timer.update(state.actual_tick);
    state.active.window->simulate(state.actual_tick);
    state.active.scene.pointer->simulate(state.actual_tick);
    post_simulate(state.actual_tick);
  }

  void game::render()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Game must be created before rendering");
    pre_render(state.alpha);
    if (!state.active.window->start_render(graphics.previous.clear.value, graphics.active.clear.value,
                                           graphics.previous.aspect.value, graphics.active.aspect.value, state.alpha))
      return;
    state.active.scene.pointer->render(state.active.window->graphics.gpu, state.active.window->graphics.command_buffer,
                                       state.active.window->graphics.render_pass, graphics.previous.aspect.value,
                                       graphics.active.aspect.value, state.alpha);
    state.active.window->end_render(state.alpha);
    post_render(state.alpha);
  }

  void game::destroy()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Game must be created before destruction");
    pre_destroy();
    state.active.scene.pointer->destroy(state.active.window->graphics.gpu);
    state.active.window->destroy();
    graphics.destroy_app();
    state.active.phase = help::phase::PREPARED;
    post_destroy();
  }

  void game::clean()
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Game must be prepared before cleaning");
    pre_clean();
    state.active.scene.pointer->clean();
    state.active.window->clean();
    state.active.phase = help::phase::CLEANED;
    post_clean();
  }

  bool game::running() { return state.active.window->state.active.running; }

  void game::time()
  {
    state.active.tick = std::max(10.0, state.active.tick);
    graphics.active.frame = std::max(1.0, graphics.active.frame);
    const double real_tick = 1.0 / state.active.tick;
    const double real_frame = 1.0 / graphics.active.frame;
    if (!equal(real_tick, state.actual_tick))
    {
      state.accumulator = state.accumulator * (real_tick / state.actual_tick);
      state.actual_tick = real_tick;
    }
    if (!equal(real_frame, graphics.actual_frame)) graphics.actual_frame = real_frame;
    state.time = static_cast<double>(SDL_GetTicksNS()) / 1e9;
    static double simulation_time{};
    double delta_time{state.time - simulation_time};
    simulation_time = state.time;
    if (delta_time > 0.1) delta_time = 0.1;
    state.accumulator += delta_time;
  }

  bool game::behind()
  {
    if (state.accumulator >= state.actual_tick)
    {
      state.accumulator -= state.actual_tick;
      return true;
    }
    return false;
  }

  void game::tps()
  {
    static double tps_time{};
    static unsigned int tick_count{};
    tick_count++;
    if (state.time - tps_time >= 1.0)
    {
      if constexpr (debug) print<CLOG>("{} TPS\n", tick_count);
      tps_time = state.time;
      tick_count = 0;
    }
  }

  bool game::ready()
  {
    static double render_time{};
    if (state.time - render_time >= graphics.actual_frame)
    {
      render_time = state.time;
      state.alpha = state.accumulator / state.actual_tick;
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
