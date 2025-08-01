#include "window.hpp"

#include <string>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_video.h"

#include "exception.hpp"

namespace cse::base
{
  window::window(const std::string &i_title, int i_starting_width, int i_starting_height, bool i_fullscreen,
                 bool i_vsync)
    : width(i_starting_width), height(i_starting_height), title(i_title), starting_width(i_starting_width),
      starting_height(i_starting_height), fullscreen(i_fullscreen), vsync(i_vsync)
  {
  }

  window::~window() { running = false; }

  void window::handle_quit() { running = false; }

  void window::handle_move()
  {
    if (fullscreen) return;

    if (!SDL_GetWindowPosition(instance, &left, &top))
      throw utility::sdl_exception("Could not get window position for window at ({}, {})", left, top);
    display_index = SDL_GetDisplayForWindow(instance);
    if (display_index == 0) throw utility::sdl_exception("Could not get display index");
  }

  void window::handle_fullscreen()
  {
    if (fullscreen)
    {
      if (!SDL_SetWindowBordered(instance, true)) throw utility::sdl_exception("Could not set window bordered");
      if (!SDL_SetWindowSize(instance, starting_width, starting_height))
        throw utility::sdl_exception("Could not set window size to ({}, {})", starting_width, starting_height);
      width = starting_width;
      height = starting_height;
      if (!SDL_SetWindowPosition(instance, left, top))
        throw utility::sdl_exception("Could not set window position to ({}, {})", left, top);
    }
    else
    {
      SDL_Rect display_bounds;
      if (!SDL_GetDisplayBounds(display_index, &display_bounds))
        throw utility::sdl_exception("Could not get display bounds for display {}", display_index);
      if (!SDL_SetWindowBordered(instance, false)) throw utility::sdl_exception("Could not set window borderless");
      if (!SDL_SetWindowSize(instance, display_bounds.w, display_bounds.h))
        throw utility::sdl_exception("Could not set window size to ({}, {}) on display {}", display_bounds.w,
                                     display_bounds.h, display_index);
      width = display_bounds.w;
      height = display_bounds.h;
      if (!SDL_SetWindowPosition(instance, SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
                                 SDL_WINDOWPOS_CENTERED_DISPLAY(display_index)))
        throw utility::sdl_exception("Could not set window position centered on display {}", display_index);
    }
    fullscreen = !fullscreen;
  }

  void window::handle_vsync()
  {
    if (vsync)
    {
      if (SDL_WindowSupportsGPUPresentMode(gpu, instance, SDL_GPU_PRESENTMODE_IMMEDIATE))
        if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                           SDL_GPU_PRESENTMODE_IMMEDIATE))
          throw utility::sdl_exception("Could not disable VSYNC for window {}", title);
    }
    else if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
      throw utility::sdl_exception("Could not enable VSYNC for window {}", title);
    vsync = !vsync;
  }
}
