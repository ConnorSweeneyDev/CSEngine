#include "window.hpp"

#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"

#include "exception.hpp"

namespace cse::core
{
  window::window(const std::string &title_, const unsigned int starting_width_, const unsigned int starting_height_,
                 const bool fullscreen_, const bool vsync_)
    : graphics(title_, starting_width_, starting_height_, fullscreen_, vsync_)
  {
    if (title_.empty())
      throw cse::utility::exception("Window title cannot be empty for window with dimensions ({}, {})", starting_width_,
                                    starting_height_);
    if (!graphics.fullscreen.on_change)
      throw cse::utility::exception("Fullscreen on_change callback must be set for window '{}'", graphics.title);
    if (!graphics.vsync.on_change)
      throw cse::utility::exception("Vsync on_change callback must be set for window '{}'", graphics.title);
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
    running = true;
  }

  void window::cleanup()
  {
    keys = nullptr;
    graphics.cleanup_gpu_and_app();
  }

  void window::input()
  {
    keys = SDL_GetKeyboardState(nullptr);
    SDL_Event event = {};
    while (SDL_PollEvent(&event)) switch (event.type)
      {
        case SDL_EVENT_QUIT: running = false; break;
        case SDL_EVENT_WINDOW_MOVED: graphics.handle_move(); break;
        case SDL_EVENT_KEY_DOWN:
          if (handle_event) handle_event(event.key);
          break;
      }
    if (handle_input) handle_input(keys);
  }

  bool window::start_render()
  {
    if (!graphics.create_command_and_swapchain()) return false;
    graphics.create_render_pass();
    return true;
  }

  void window::end_render() { graphics.end_render_and_submit_command(); }
}
