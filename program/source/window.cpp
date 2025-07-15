#include "window.hpp"

#include <string>

#include "SDL3/SDL_error.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_video.h"

namespace cse
{
  Window::Window(const std::string &title, int i_width, int i_height)
    : width(i_width), height(i_height), starting_width(i_width), starting_height(i_height)
  {
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
      SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
      return;
    }

    handle = SDL_CreateWindow(title.c_str(), i_width, i_height, 0);
    if (handle == nullptr)
    {
      SDL_Log("Failed to create window: %s", SDL_GetError());
      return;
    }

    display_index = SDL_GetPrimaryDisplay();
    if (display_index == 0)
    {
      SDL_Log("Failed to get primary display: %s", SDL_GetError());
      return;
    }
    left = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    top = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    if (!SDL_SetWindowPosition(handle, left, top))
    {
      SDL_Log("Failed to set window position: %s", SDL_GetError());
      return;
    }
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
    if (!SDL_GetWindowPosition(handle, &left, &top))
    {
      SDL_Log("Failed to get window position: %s", SDL_GetError());
      return false;
    }
    display_index = SDL_GetDisplayForWindow(handle);
    if (display_index == 0)
    {
      SDL_Log("Failed to get display for window: %s", SDL_GetError());
      return false;
    }
    return true;
  }

  bool Window::handle_fullscreen()
  {
    if (fullscreen)
    {
      if (!SDL_SetWindowBordered(handle, true))
      {
        SDL_Log("Failed to set window bordered: %s", SDL_GetError());
        return false;
      }
      if (!SDL_SetWindowSize(handle, starting_width, starting_height))
      {
        SDL_Log("Failed to set window size: %s", SDL_GetError());
        return false;
      }
      if (!SDL_SetWindowPosition(handle, left, top))
      {
        SDL_Log("Failed to set window position: %s", SDL_GetError());
        return false;
      }
    }
    else
    {
      SDL_Rect display_bounds;
      if (!SDL_GetDisplayBounds(display_index, &display_bounds))
      {
        SDL_Log("Failed to get display bounds: %s", SDL_GetError());
        return false;
      }
      if (!SDL_SetWindowBordered(handle, false))
      {
        SDL_Log("Failed to set window bordered: %s", SDL_GetError());
        return false;
      }
      if (!SDL_SetWindowSize(handle, display_bounds.w, display_bounds.h))
      {
        SDL_Log("Failed to set window size: %s", SDL_GetError());
        return false;
      }
      if (!SDL_SetWindowPosition(handle, SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
                                 SDL_WINDOWPOS_CENTERED_DISPLAY(display_index)))
      {
        SDL_Log("Failed to set window position: %s", SDL_GetError());
        return false;
      }
    }
    fullscreen = !fullscreen;
    return true;
  }
}
