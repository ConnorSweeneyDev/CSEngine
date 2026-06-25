#include "window.hpp"

#include <string>

#include "SDL3/SDL_events.h"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"

#include "exception.hpp"
#include "game.hpp"
#include "state.hpp"

namespace cse
{
  window::window(const initial_state &state_, const initial_graphics &graphics_)
    : state{state_.display, state_.left, state_.top, state_.width, state_.height, state_.mouse},
      graphics{graphics_.title, graphics_.fullscreen, graphics_.vsync}
  {
  }

  void window::on_prepare() {}
  void window::prepare()
  {
    if (state.active.phase != help::phase::CLEANED) throw exception("Window must be cleaned before preparation");
    state.active.running = true;
    state.active.phase = help::phase::PREPARED;
    on_prepare();
  }

  void window::on_create() {}
  void window::create()
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Window must be prepared before creation");
    graphics.create(state.active.display, state.active.left, state.active.top, state.active.width, state.active.height,
                    PRIMARY, CENTER);
    state.create(graphics.instance, game->graphics.active.aspect.value, game->graphics.active.resolution);
    state.active.phase = help::phase::CREATED;
    on_create();
  }

  void window::on_synchronize() {}
  void window::synchronize()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Window must be created before synchronization");
    state.synchronize();
    graphics.synchronize();
    on_synchronize();
  }

  void window::on_event(const SDL_Event &) {}
  void window::event()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Window must be created before processing events");
    switch (state.event.type)
    {
      case SDL_EVENT_QUIT: state.active.running = false; break;
      case SDL_EVENT_WINDOW_MOVED:
        graphics.handle_move(state.active.display, state.active.left, state.active.top);
        break;
      case SDL_EVENT_WINDOW_RESIZED:
        graphics.handle_resize(state.active.display, state.active.left, state.active.top, state.active.width,
                               state.active.height);
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        state.active.mouse.wheel += glm::dvec2{state.event.wheel.x, state.event.wheel.y};
        on_event(state.event);
        break;
      default: on_event(state.event); break;
    }
  }

  void window::on_simulate(const double) {}
  void window::simulate(const double tick)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Window must be created before simulation");
    state.active.timer.update(tick);
    on_simulate(tick);
  }

  void window::pre_render(const double) {}
  bool window::start_render(const glm::dvec3 &previous_clear, const glm::dvec3 &active_clear,
                            const double previous_aspect, const double active_aspect, const double alpha)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Window must be created before pre-rendering");
    graphics.reconcile(state.active.display, state.active.left, state.active.top, state.active.width,
                       state.active.height, PRIMARY, CENTER);
    if (!graphics.acquire_swapchain_texture()) return false;
    pre_render(alpha);
    graphics.start_render_pass(state.active.width, state.active.height, previous_clear, active_clear, previous_aspect,
                               active_aspect, alpha);
    return true;
  }

  void window::post_render(const double) {}
  void window::end_render(const double alpha)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Window must be created before post-rendering");
    graphics.end_render_pass();
    post_render(alpha);
  }

  void window::on_destroy() {}
  void window::destroy()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Window must be created before destruction");
    state.event = {};
    graphics.destroy();
    state.active.phase = help::phase::PREPARED;
    on_destroy();
  }

  void window::on_clean() {}
  void window::clean()
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Window must be prepared before cleaning");
    state.active.phase = help::phase::CLEANED;
    on_clean();
  }
}
