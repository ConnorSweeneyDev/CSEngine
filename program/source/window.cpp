#include "window.hpp"

#include <string>

#include "SDL3/SDL_events.h"
#include "glm/ext/vector_double4.hpp"

#include "exception.hpp"
#include "state.hpp"

namespace cse
{
  window::window(const initial_state &state_, const initial_graphics &graphics_)
    : state{state_.width, state_.height, state_.fullscreen, state_.vsync}, graphics{graphics_.title}
  {
    state.active.width.change = [this]()
    { graphics.handle_manual_resize(state.active.width, state.active.height, state.active.fullscreen); };
    state.active.height.change = [this]()
    { graphics.handle_manual_resize(state.active.width, state.active.height, state.active.fullscreen); };
    state.active.left.change = [this]()
    { graphics.handle_manual_move(state.active.left, state.active.top, state.active.fullscreen); };
    state.active.top.change = [this]()
    { graphics.handle_manual_move(state.active.left, state.active.top, state.active.fullscreen); };
    state.active.display_index.change = [this]()
    {
      graphics.handle_manual_display_move(state.active.width, state.active.height, state.active.left, state.active.top,
                                          state.active.display_index, state.active.fullscreen);
    };
    state.active.fullscreen.change = [this]()
    { graphics.handle_fullscreen(state.active.fullscreen, state.active.display_index); };
    state.active.vsync.change = [this]() { graphics.handle_vsync(state.active.vsync); };
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
    graphics.create_window(state.active.width, state.active.height, state.active.left, state.active.top,
                           state.active.display_index, state.active.fullscreen, state.active.vsync);
    state.active.phase = help::phase::CREATED;
    on_create();
  }

  void window::on_previous() {}
  void window::previous()
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Window must be created before updating previous state");
    state.update_previous();
    graphics.update_previous();
    on_previous();
  }

  void window::on_event(const SDL_Event &) {}
  void window::event()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Window must be created before processing events");
    switch (state.event.type)
    {
      case SDL_EVENT_QUIT: state.active.running = false; break;
      case SDL_EVENT_WINDOW_MOVED:
        graphics.handle_move(state.active.left, state.active.top, state.active.display_index, state.active.fullscreen);
        break;
      case SDL_EVENT_WINDOW_RESIZED:
        graphics.handle_resize(state.active.width, state.active.height, state.active.display_index,
                               state.active.fullscreen);
        break;
      default: on_event(state.event); break;
    }
  }

  void window::on_input(const bool *) {}
  void window::input()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Window must be created before processing input");
    on_input(state.input);
  }

  void window::on_simulate(const double) {}
  void window::simulate(const double tick)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Window must be created before simulation");
    state.active.timer.update(tick);
    on_simulate(tick);
  }

  void window::pre_render(const double) {}
  bool window::start_render(const glm::dvec4 &previous_clear, const glm::dvec4 &active_clear,
                            const double previous_aspect, const double active_aspect, const double alpha)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Window must be created before pre-rendering");
    if (!graphics.acquire_swapchain_texture()) return false;
    graphics.start_render_pass(state.active.width, state.active.height, previous_clear, active_clear, previous_aspect,
                               active_aspect, alpha);
    pre_render(alpha);
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
    state.input = nullptr;
    state.event = {};
    graphics.destroy_window();
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
