#include "window.hpp"

#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"

#include "exception.hpp"

namespace cse::core
{
  window::window(const std::string &title_, const unsigned int width_, const unsigned int height_,
                 const bool fullscreen_, const bool vsync_)
    : title(title_), width(width_), height(height_), fullscreen(fullscreen_), vsync(vsync_)
  {
    if (title_.empty()) throw cse::utility::exception("Window title cannot be empty");
    width.on_change = [this]() { graphics.handle_manual_resize(width, height, fullscreen); };
    height.on_change = [this]() { graphics.handle_manual_resize(width, height, fullscreen); };
    left.on_change = [this]() { graphics.handle_manual_move(left, top, fullscreen); };
    top.on_change = [this]() { graphics.handle_manual_move(left, top, fullscreen); };
    display_index.on_change = [this]()
    { graphics.handle_manual_display_move(width, height, left, top, display_index, fullscreen); };
    fullscreen.on_change = [this]() { graphics.handle_fullscreen(fullscreen, left, top, display_index); };
    vsync.on_change = [this]() { graphics.handle_vsync(vsync); };
  }

  window::~window()
  {
    vsync.on_change = nullptr;
    fullscreen.on_change = nullptr;
    display_index.on_change = nullptr;
    top.on_change = nullptr;
    left.on_change = nullptr;
    height.on_change = nullptr;
    width.on_change = nullptr;
    input_hooks.clear();
    event_hooks.clear();
  }

  void window::initialize()
  {
    graphics.create_app_and_window(title, width, height, left, top, display_index, fullscreen, vsync);
    running = true;
  }

  void window::cleanup()
  {
    current_keys = nullptr;
    current_event = {};
    graphics.destroy_window_and_app();
  }

  void window::event()
  {
    switch (current_event.type)
    {
      case SDL_EVENT_QUIT: running = false; break;
      case SDL_EVENT_WINDOW_MOVED: graphics.handle_move(left, top, display_index, fullscreen); break;
      case SDL_EVENT_WINDOW_RESIZED: graphics.handle_resize(width, height, display_index, fullscreen); break;
      default: event_hooks.call("main", current_event); break;
    }
  }

  void window::input()
  {
    current_keys = SDL_GetKeyboardState(nullptr);
    input_hooks.call("main", current_keys);
  }

  bool window::start_render(const float target_aspect_ratio)
  {
    if (!graphics.acquire_swapchain_texture()) return false;
    graphics.start_render_pass(target_aspect_ratio, width, height);
    return true;
  }

  void window::end_render() { graphics.end_render_pass(); }
}
