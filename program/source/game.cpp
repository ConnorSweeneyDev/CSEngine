#include "game.hpp"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_timer.h"

#include "container.hpp"
#include "exception.hpp"
#include "interface.hpp"
#include "name.hpp"
#include "numeric.hpp"
#include "print.hpp"
#include "scene.hpp"
#include "state.hpp"
#include "window.hpp"

namespace cse
{
  game::game(const initial_state &state_, const initial_graphics &graphics_, const initial_audio &audio_)
    : state{state_.tick}, graphics{graphics_.frame, graphics_.aspect, graphics_.resolution, graphics_.clear},
      audio{audio_.master, audio_.sound, audio_.music}
  {
  }

  game &game::current(const name scene_name)
  {
    auto scene{throw_find(state.active.scenes, scene_name)};
    if (state.active.phase == help::phase::CREATED)
      state.next.scene = {scene_name, {}};
    else
    {
      state.active.scene = scene;
      state.previous.scene = scene;
    }
    return *this;
  }

  void game::run()
  {
    prepare();
    create();
    while (running())
    {
      step();
      while (behind())
      {
        tps();
        previous();
        sync();
        event();
        input();
        simulate();
        collide();
        tps();
      }
      if (ready())
      {
        fps();
        render();
        mix();
        fps();
      }
    }
    destroy();
    clean();
  }

  void game::pre_prepare() {}
  void game::post_prepare() {}
  void game::prepare()
  {
    if (state.active.phase != help::phase::CLEANED) throw exception("Game must be cleaned before preparation");
    if (!state.active.window) throw exception("No window has been set for the game");
    if (!state.active.window->game) state.active.window->game = this;
    for (const auto &scene : state.active.scenes)
      if (!scene->game) scene->game = this;
    for (const auto &interface : state.active.interfaces)
      if (!interface->game) interface->game = this;
    pre_prepare();
    state.active.window->prepare();
    if (state.active.scenes.empty()) throw exception("No scenes have been added to the game");
    if (!state.active.scene) throw exception("No current scene has been set for the game");
    for (const auto &scene : state.active.scenes) scene->prepare();
    for (const auto &interface : state.active.interfaces) interface->prepare();
    state.active.phase = help::phase::PREPARED;
    post_prepare();
  }

  void game::pre_create() {}
  void game::post_create() {}
  void game::create()
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Game must be prepared before creation");
    pre_create();
    graphics.create_app();
    audio.create_app();
    state.active.window->create();
    graphics.create(state.active.window->graphics.gpu);
    state.active.scene->create();
    for (const auto &interface : state.active.interfaces) interface->create();
    state.active.phase = help::phase::CREATED;
    post_create();
  }

  void game::pre_previous() {}
  void game::post_previous() {}
  void game::previous()
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Game must be created before updating previous state");
    pre_previous();
    state.update_previous();
    graphics.update_previous();
    audio.update_previous();
    state.active.window->previous();
    state.active.scene->previous();
    for (const auto &interface : state.active.interfaces) interface->previous();
    post_previous();
  }

  void game::pre_sync() {}
  void game::post_sync() {}
  void game::sync()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Game must be created before synchronization");
    pre_sync();
    if (state.next.window.has_value())
    {
      if (auto &window{state.next.window.value()})
      {
        for (const auto &interface : state.active.interfaces) interface->destroy();
        state.active.scene->destroy();
        graphics.destroy(state.active.window->graphics.gpu);
        state.active.window->destroy();
        state.active.window->clean();
        state.active.window = window;
        window->prepare();
        window->create();
        graphics.create(state.active.window->graphics.gpu);
        state.active.scene->create();
        for (const auto &interface : state.active.interfaces) interface->create();
      }
      else
        throw exception("Tried to set window to null");
      state.next.window.reset();
    }
    if (state.next.scene.has_value())
    {
      if (auto &[name, scene]{state.next.scene.value()}; scene)
      {
        state.active.scene->destroy();
        if (name == state.active.scene->name) state.active.scene->clean();
        set_or_add(state.active.scenes, scene);
        state.active.scene = scene;
        scene->prepare();
        scene->create();
      }
      else
      {
        auto next_scene{throw_find(state.active.scenes, name)};
        if (name != state.active.scene->name)
        {
          state.active.scene->destroy();
          state.active.scene = next_scene;
          next_scene->create();
        }
      }
      state.next.scene.reset();
    }
    state.active.scene->sync();
    if (!state.interface_removals.empty())
    {
      for (const auto &interface_name : state.interface_removals)
        if (auto iterator{try_iterate(state.active.interfaces, interface_name)};
            iterator != state.active.interfaces.end())
        {
          const auto &interface{*iterator};
          if (interface->state.active.phase == help::phase::CREATED) interface->destroy();
          interface->clean();
          state.active.interfaces.erase(iterator);
        }
      state.interface_removals.clear();
    }
    if (!state.interface_additions.empty())
    {
      for (auto &interface : state.interface_additions)
      {
        set_or_add(state.active.interfaces, interface);
        interface->prepare();
        interface->create();
      }
      state.interface_additions.clear();
    }
    state.generate_order(state.active.interfaces);
    state.generate_pool(state.active.scene->state.interface_order);
    post_sync();
  }

  void game::pre_event(const SDL_Event &) {}
  void game::post_event(const SDL_Event &) {}
  void game::event()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Game must be created before processing events");
    state.active.window->state.poll_mouse(state.active.window->graphics.instance, graphics.active.aspect.value,
                                          graphics.active.resolution);
    state.reset_targets();
    while (SDL_PollEvent(&state.active.window->state.event))
    {
      pre_event(state.active.window->state.event);
      state.interact(state.active.window->state.event, graphics.active.aspect.value, graphics.active.resolution);
      state.active.window->event();
      state.active.scene->event(state.active.window->state.event);
      for (const auto &interface : state.order) interface->event(state.active.window->state.event);
      post_event(state.active.window->state.event);
    }
  }

  void game::pre_input() {}
  void game::post_input() {}
  void game::input()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Game must be created before processing input");
    state.active.window->state.poll_keyboard();
    pre_input();
    state.hover(state.active.window->graphics.instance, graphics.active.aspect.value, graphics.active.resolution);
    post_input();
  }

  void game::pre_simulate(const double) {}
  void game::post_simulate(const double) {}
  void game::simulate()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Game must be created before simulation");
    pre_simulate(state.actual_tick);
    state.active.timer.update(state.actual_tick);
    state.active.window->simulate(state.actual_tick);
    state.active.scene->simulate(state.actual_tick);
    for (const auto &interface : state.order) interface->simulate(state.actual_tick);
    post_simulate(state.actual_tick);
  }

  void game::pre_collide(const double) {}
  void game::post_collide(const double) {}
  void game::collide()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Game must be created before collision");
    pre_collide(state.actual_tick);
    state.active.scene->collide(state.actual_tick);
    post_collide(state.actual_tick);
  }

  void game::pre_render(const double) {}
  void game::post_render(const double) {}
  void game::render()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Game must be created before rendering");
    pre_render(state.alpha);
    if (!state.active.window->start_render(graphics.previous.clear.value, graphics.active.clear.value,
                                           graphics.previous.aspect.value, graphics.active.aspect.value, state.alpha))
      return;
    state.active.scene->render(state.active.window->graphics.instance, state.active.window->graphics.gpu,
                               state.active.window->graphics.command_buffer, state.active.window->graphics.render_pass,
                               graphics.previous.aspect.value, graphics.active.aspect.value, state.alpha);
    graphics.render(state.active.scene->state.active.interfaces, state.active.interfaces,
                    state.active.window->graphics.instance, state.active.window->graphics.gpu,
                    state.active.window->graphics.command_buffer, state.active.window->graphics.render_pass,
                    state.alpha);
    state.active.window->end_render(state.alpha);
    post_render(state.alpha);
  }

  void game::pre_mix(const double) {}
  void game::post_mix(const double) {}
  void game::mix()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Game must be created before mixing");
    pre_mix(state.alpha);
    audio.mix(state.previous.mixer, state.active.mixer, state.active.window, state.active.interfaces,
              state.active.scene, state.alpha);
    post_mix(state.alpha);
  }

  void game::pre_destroy() {}
  void game::post_destroy() {}
  void game::destroy()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Game must be created before destruction");
    pre_destroy();
    for (const auto &interface : state.active.interfaces) interface->destroy();
    state.active.scene->destroy();
    graphics.destroy(state.active.window->graphics.gpu);
    state.active.window->destroy();
    audio.destroy_app();
    graphics.destroy_app();
    state.active.phase = help::phase::PREPARED;
    post_destroy();
  }

  void game::pre_clean() {}
  void game::post_clean() {}
  void game::clean()
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Game must be prepared before cleaning");
    pre_clean();
    for (const auto &interface : state.active.interfaces) interface->clean();
    for (const auto &scene : state.active.scenes) scene->clean();
    state.active.window->clean();
    state.active.phase = help::phase::CLEANED;
    post_clean();
  }

  void game::time() { state.time = static_cast<double>(SDL_GetTicksNS()) / 1e9; }

  void game::step()
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

    time();
    static double simulation_time{};
    double delta_time{state.time - simulation_time};
    simulation_time = state.time;
    if (delta_time > 0.1) delta_time = 0.1;
    state.accumulator += delta_time;
  }

  bool game::running() { return state.active.window->state.active.running; }

  bool game::behind()
  {
    if (state.accumulator >= state.actual_tick)
    {
      state.accumulator -= state.actual_tick;
      return true;
    }
    return false;
  }

  bool game::ready()
  {
    time();
    static double deadline{};
    if (state.time - deadline >= graphics.actual_frame)
    {
      deadline += graphics.actual_frame;
      if (state.time - deadline >= graphics.actual_frame) deadline = state.time;
      state.alpha = state.accumulator / state.actual_tick;
      return true;
    }
    return false;
  }

  void game::tps()
  {
    static std::optional<double> start{};
    static double deadline{};
    static double accumulator{};
    static unsigned int count{};
    if (!start)
    {
      start = static_cast<double>(SDL_GetTicksNS()) / 1e9;
      return;
    }

    count++;
    accumulator += (static_cast<double>(SDL_GetTicksNS()) / 1e9) - start.value();
    if (state.time - deadline >= 1.0)
    {
      const double average = (accumulator / count) * 1000.0;
      const double target = state.actual_tick * 1000.0;
      const double usage = (average / target) * 100.0;
      print<CLOG>("TPS: {} | {:.3f}ms / {:.3f}ms ({:.2f}%)\n", count, average, target, usage);
      deadline = state.time;
      accumulator = 0.0;
      count = 0;
    }
    start.reset();
  }

  void game::fps()
  {
    static std::optional<double> start{};
    static double deadline{};
    static double accumulator{};
    static unsigned int count{};
    if (!start)
    {
      start = static_cast<double>(SDL_GetTicksNS()) / 1e9;
      return;
    }

    count++;
    accumulator += (static_cast<double>(SDL_GetTicksNS()) / 1e9) - start.value();
    if (state.time - deadline >= 1.0)
    {
      const double average = (accumulator / count) * 1000.0;
      const double target = graphics.actual_frame * 1000.0;
      const double usage = (average / target) * 100.0;
      print<CLOG>("FPS: {} | {:.3f}ms / {:.3f}ms ({:.2f}%)\n", count, average, target, usage);
      deadline = state.time;
      accumulator = 0.0;
      count = 0;
    }
    start.reset();
  }
}
