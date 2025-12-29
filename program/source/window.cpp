#include "window.hpp"

#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"
#include "glm/ext/vector_uint2.hpp"

#include "exception.hpp"

namespace cse
{
  window::window(const std::string &title_, const glm::uvec2 &dimensions_, const bool fullscreen_, const bool vsync_)
    : state(dimensions_, fullscreen_, vsync_), graphics(title_)
  {
    if (graphics.title.empty()) throw exception("Window title cannot be empty");
    state.width.on_change = [this]() { graphics.handle_manual_resize(state.width, state.height, state.fullscreen); };
    state.height.on_change = [this]() { graphics.handle_manual_resize(state.width, state.height, state.fullscreen); };
    state.left.on_change = [this]() { graphics.handle_manual_move(state.left, state.top, state.fullscreen); };
    state.top.on_change = [this]() { graphics.handle_manual_move(state.left, state.top, state.fullscreen); };
    state.display_index.on_change = [this]()
    {
      graphics.handle_manual_display_move(state.width, state.height, state.left, state.top, state.display_index,
                                          state.fullscreen);
    };
    state.fullscreen.on_change = [this]() { graphics.handle_fullscreen(state.fullscreen, state.display_index); };
    state.vsync.on_change = [this]() { graphics.handle_vsync(state.vsync); };
  }

  window::~window()
  {
    current_keys = nullptr;
    hooks.clear();
    state.vsync.on_change = nullptr;
    state.fullscreen.on_change = nullptr;
    state.display_index.on_change = nullptr;
    state.top.on_change = nullptr;
    state.left.on_change = nullptr;
    state.height.on_change = nullptr;
    state.width.on_change = nullptr;
    parent.reset();
  }

  void window::initialize()
  {
    graphics.create_app_and_window(state.width, state.height, state.left, state.top, state.display_index,
                                   state.fullscreen, state.vsync);
    state.running = true;
    hooks.call<void()>("initialize");
  }

  void window::event()
  {
    switch (current_event.type)
    {
      case SDL_EVENT_QUIT: state.running = false; break;
      case SDL_EVENT_WINDOW_MOVED:
        graphics.handle_move(state.left, state.top, state.display_index, state.fullscreen);
        break;
      case SDL_EVENT_WINDOW_RESIZED:
        graphics.handle_resize(state.width, state.height, state.display_index, state.fullscreen);
        break;
      default: hooks.call<void(const SDL_Event &)>("event", current_event); break;
    }
  }

  void window::input()
  {
    current_keys = SDL_GetKeyboardState(nullptr);
    hooks.call<void(const bool *)>("input", current_keys);
  }

  void window::simulate() { hooks.call<void()>("simulate"); }

  bool window::start_render(const float aspect_ratio)
  {
    if (!graphics.acquire_swapchain_texture()) return false;
    graphics.start_render_pass(aspect_ratio, state.width, state.height);
    hooks.call<void(const unsigned int, const unsigned int)>("pre_render", state.width, state.height);
    return true;
  }

  void window::end_render()
  {
    graphics.end_render_pass();
    hooks.call<void()>("post_render");
  }

  void window::cleanup()
  {
    current_keys = nullptr;
    current_event = {};
    graphics.destroy_window_and_app();
    hooks.call<void()>("cleanup");
  }
}
