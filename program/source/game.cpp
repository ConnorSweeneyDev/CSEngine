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
  game::game(const double poll_rate_, const double frame_rate_, const double aspect_ratio_,
             const glm::dvec4 &clear_color_)
    : state{poll_rate_}, graphics{frame_rate_, aspect_ratio_, clear_color_}
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
    pre_simulate(state.actual_poll_rate);
    state.active.timer.update(state.actual_poll_rate);
    state.active.window->simulate(state.actual_poll_rate);
    state.active.scene.pointer->simulate(state.actual_poll_rate);
    post_simulate(state.actual_poll_rate);
  }

  void game::render()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Game must be created before rendering");
    pre_render(state.alpha);
    if (!state.active.window->start_render(graphics.previous.clear_color.value, graphics.active.clear_color.value,
                                           state.alpha, graphics.previous.aspect_ratio.value,
                                           graphics.active.aspect_ratio.value))
      return;
    state.active.scene.pointer->render(state.active.window->graphics.gpu, state.active.window->graphics.command_buffer,
                                       state.active.window->graphics.render_pass, state.alpha,
                                       graphics.previous.aspect_ratio.value, graphics.active.aspect_ratio.value);
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
