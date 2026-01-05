#include "window.hpp"

#include <string>

#include "SDL3/SDL_events.h"
#include "glm/ext/vector_uint2.hpp"

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

  void window::initialize()
  {
    graphics.create_window(state.active.width, state.active.height, state.active.left, state.active.top,
                           state.active.display_index, state.active.fullscreen, state.active.vsync);
    state.active.running = true;
    state.initialized = true;
    hook.call<void()>("initialize");
  }

  void window::event()
  {
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

  void window::input() { hook.call<void(const bool *)>("input", state.keys); }

  void window::simulate(const float poll_rate) { hook.call<void(const float)>("simulate", poll_rate); }

  bool window::start_render(const double alpha, const float aspect_ratio)
  {
    if (!graphics.acquire_swapchain_texture()) return false;
    graphics.start_render_pass(state.active.width, state.active.height, aspect_ratio);
    hook.call<void(const double)>("pre_render", alpha);
    return true;
  }

  void window::end_render(const double alpha)
  {
    graphics.end_render_pass();
    hook.call<void(const double)>("post_render", alpha);
  }

  void window::cleanup()
  {
    state.keys = nullptr;
    state.event = {};
    graphics.destroy_window();
    state.initialized = false;
    hook.call<void()>("cleanup");
  }

  void window::previous()
  {
    state.update_previous();
    graphics.update_previous();
  }
}
