#include "window.hpp"

#include <string>

#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_video.h"

namespace cse
{
  Window::Window(const std::string &title, int i_width, int i_height)
    : width(i_width), height(i_height), starting_width(i_width), starting_height(i_height)
  {
    handle = SDL_CreateWindow(title.c_str(), i_width, i_height, SDL_WINDOWPOS_CENTERED);
    if (handle == nullptr) return;

    display_index = SDL_GetPrimaryDisplay();
    if (display_index == 0) return;
    left = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    top = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    if (!SDL_SetWindowPosition(handle, left, top)) return;
    if (fullscreen)
      if (!handle_fullscreen()) return;

    initialized = true;
  }

  Window::~Window()
  {
    if (!handle) return;
    SDL_DestroyWindow(handle);
    handle = nullptr;
  }

  bool Window::handle_move()
  {
    if (fullscreen) return true;
    if (!SDL_GetWindowPosition(handle, &left, &top)) return false;
    display_index = SDL_GetDisplayForWindow(handle);
    if (display_index == 0) return false;
    return true;
  }

  bool Window::handle_fullscreen()
  {
    if (fullscreen)
    {
      if (!SDL_SetWindowBordered(handle, true)) return false;
      if (!SDL_SetWindowSize(handle, starting_width, starting_height)) return false;
      if (!SDL_SetWindowPosition(handle, left, top)) return false;
    }
    else
    {
      SDL_Rect display_bounds;
      if (!SDL_GetDisplayBounds(display_index, &display_bounds)) return false;
      if (!SDL_SetWindowBordered(handle, false)) return false;
      if (!SDL_SetWindowSize(handle, display_bounds.w, display_bounds.h)) return false;
      if (!SDL_SetWindowPosition(handle, SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
                                 SDL_WINDOWPOS_CENTERED_DISPLAY(display_index)))
        return false;
    }
    fullscreen = !fullscreen;
    return true;
  }
}
