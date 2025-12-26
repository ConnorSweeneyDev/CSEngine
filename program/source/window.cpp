#include "window.hpp"

#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"
#include "glm/ext/vector_uint2.hpp"

#include "exception.hpp"

namespace cse
{
  window::window(const std::string &title_, const glm::uvec2 &dimensions_, const bool fullscreen_, const bool vsync_)
    : title(title_), width(dimensions_.x), height(dimensions_.y), fullscreen(fullscreen_), vsync(vsync_)
  {
    if (title_.empty()) throw exception("Window title cannot be empty");
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
    current_keys = nullptr;
    hooks.clear();
    vsync.on_change = nullptr;
    fullscreen.on_change = nullptr;
    display_index.on_change = nullptr;
    top.on_change = nullptr;
    left.on_change = nullptr;
    height.on_change = nullptr;
    width.on_change = nullptr;
    game.reset();
  }

  void window::initialize()
  {
    graphics.create_app_and_window(title, width, height, left, top, display_index, fullscreen, vsync);
    running = true;
    hooks.call<void()>("initialize_main");
  }

  void window::event()
  {
    switch (current_event.type)
    {
      case SDL_EVENT_QUIT: running = false; break;
      case SDL_EVENT_WINDOW_MOVED: graphics.handle_move(left, top, display_index, fullscreen); break;
      case SDL_EVENT_WINDOW_RESIZED: graphics.handle_resize(width, height, display_index, fullscreen); break;
      default: hooks.call<void(const SDL_Event &)>("event_main", current_event); break;
    }
  }

  void window::input()
  {
    current_keys = SDL_GetKeyboardState(nullptr);
    hooks.call<void(const bool *)>("input_main", current_keys);
  }

  void window::simulate() { hooks.call<void()>("simulate_main"); }

  bool window::start_render(const float target_aspect_ratio)
  {
    if (!graphics.acquire_swapchain_texture()) return false;
    graphics.start_render_pass(target_aspect_ratio, width, height);
    return true;
  }

  void window::end_render() { graphics.end_render_pass(); }

  void window::cleanup()
  {
    current_keys = nullptr;
    current_event = {};
    graphics.destroy_window_and_app();
  }
}
