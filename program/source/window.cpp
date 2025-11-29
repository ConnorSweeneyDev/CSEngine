#include "window.hpp"

#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"

#include "utility.hpp"

namespace cse::core
{
  window::window(const std::string &title_, const unsigned int starting_width_, const unsigned int starting_height_,
                 const bool fullscreen_, const bool vsync_)
    : graphics(title_, starting_width_, starting_height_, fullscreen_, vsync_)
  {
    if (title_.empty())
      throw cse::utility::exception("Window title cannot be empty for window with dimensions ({}, {})", starting_width_,
                                    starting_height_);
  }

  window::~window()
  {
    handle_event = nullptr;
    handle_input = nullptr;
  }

  void window::initialize()
  {
    graphics.initialize_app();
    graphics.create_window();
    state.running = true;
  }

  void window::cleanup()
  {
    state.keys = nullptr;
    graphics.cleanup_gpu_and_app();
  }

  void window::event()
  {
    switch (state.event.type)
    {
      case SDL_EVENT_QUIT: state.running = false; break;
      case SDL_EVENT_WINDOW_MOVED: graphics.handle_move(); break;
      case SDL_EVENT_WINDOW_RESIZED: graphics.handle_resize(); break;
      default:
        if (handle_event) handle_event(state.event);
        break;
    }
  }

  void window::input()
  {
    state.keys = SDL_GetKeyboardState(nullptr);
    if (handle_input) handle_input(state.keys);
  }

  bool window::start_render(const float target_aspect_ratio)
  {
    if (!graphics.create_command_and_swapchain()) return false;
    graphics.create_render_pass(target_aspect_ratio);
    return true;
  }

  void window::end_render() { graphics.end_render_and_submit_command(); }
}
