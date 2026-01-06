#include "window.hpp"

#include <string>

#include "SDL3/SDL_events.h"
#include "glm/ext/vector_uint2.hpp"

#include "exception.hpp"

namespace cse
{
  window::window(const std::string &title_, const glm::uvec2 &dimensions_, const bool fullscreen_, const bool vsync_)
    : state{dimensions_, fullscreen_, vsync_}, graphics{title_}
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

  window::~window() { hook.reset(); }

  void window::prepare()
  {
    if (state.prepared) throw exception("Window cannot be prepared more than once");
    if (state.created) throw exception("Window cannot be prepared while created");
    state.active.running = true;
    state.prepared = true;
    hook.call<void()>("prepare");
  }

  void window::create()
  {
    if (!state.prepared) throw exception("Window must be prepared before creation");
    if (state.created) throw exception("Window cannot be created more than once");
    graphics.create_window(state.active.width, state.active.height, state.active.left, state.active.top,
                           state.active.display_index, state.active.fullscreen, state.active.vsync);
    state.created = true;
    hook.call<void()>("create");
  }

  void window::previous()
  {
    if (!state.prepared) throw exception("Window must be prepared before updating previous state");
    if (!state.created) throw exception("Window must be created before updating previous state");
    state.update_previous();
    graphics.update_previous();
  }

  void window::event()
  {
    if (!state.prepared) throw exception("Window must be prepared before processing events");
    if (!state.created) throw exception("Window must be created before processing events");
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
      default: hook.call<void(const SDL_Event &)>("event", state.event); break;
    }
  }

  void window::input()
  {
    if (!state.prepared) throw exception("Window must be prepared before processing input");
    if (!state.created) throw exception("Window must be created before processing input");
    hook.call<void(const bool *)>("input", state.input);
  }

  void window::simulate(const float poll_rate)
  {
    if (!state.prepared) throw exception("Window must be prepared before simulation");
    if (!state.created) throw exception("Window must be created before simulation");
    hook.call<void(const float)>("simulate", poll_rate);
  }

  bool window::pre_render(const double alpha, const float aspect_ratio)
  {
    if (!state.prepared) throw exception("Window must be prepared before starting rendering");
    if (!state.created) throw exception("Window must be created before starting rendering");
    if (!graphics.acquire_swapchain_texture()) return false;
    graphics.start_render_pass(state.active.width, state.active.height, aspect_ratio);
    hook.call<void(const double)>("pre_render", alpha);
    return true;
  }

  void window::post_render(const double alpha)
  {
    if (!state.prepared) throw exception("Window must be prepared before ending rendering");
    if (!state.created) throw exception("Window must be created before ending rendering");
    graphics.end_render_pass();
    hook.call<void(const double)>("post_render", alpha);
  }

  void window::destroy()
  {
    if (!state.prepared) throw exception("Window must be prepared before destruction");
    if (!state.created) throw exception("Window cannot be destroyed more than once");
    state.input = nullptr;
    state.event = {};
    graphics.destroy_window();
    state.created = false;
    hook.call<void()>("destroy");
  }

  void window::clean()
  {
    if (!state.prepared) throw exception("Window cannot be cleaned more than once");
    if (state.created) throw exception("Window must be destroyed before cleaning");
    state.prepared = false;
    hook.call<void()>("clean");
  }
}
