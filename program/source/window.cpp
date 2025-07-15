#include "window.hpp"

#include <string>

#include "SDL3/SDL_init.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_video.h"

#include "utility.hpp"

namespace cse
{
  Window::Window(const std::string &title, int i_width, int i_height)
    : width(i_width), height(i_height), starting_width(i_width), starting_height(i_height)
  {
    handle = SDL_CreateWindow(title.c_str(), i_width, i_height, 0);
    if (handle == nullptr)
    {
      utility::log("Could not create window", utility::SDL_FAILURE);
      return;
    }

    display_index = SDL_GetPrimaryDisplay();
    if (display_index == 0)
    {
      utility::log("Could not get primary display", utility::SDL_FAILURE);
      return;
    }
    left = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    top = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);

    if (!SDL_SetWindowPosition(handle, left, top))
    {
      utility::log("Could not set window position", utility::SDL_FAILURE);
      return;
    }
    if (fullscreen)
      if (!handle_fullscreen())
      {
        utility::log("Could not handle fullscreen", utility::SDL_FAILURE);
        return;
      }

    initialized = true;
  }

  Window::~Window()
  {
    if (!handle) return;

    SDL_DestroyWindow(handle);
    handle = nullptr;
    initialized = false;
  }

  SDL_AppResult Window::handle_move()
  {
    if (fullscreen) return SDL_APP_CONTINUE;

    if (!SDL_GetWindowPosition(handle, &left, &top))
      return utility::log("Could not get window position", utility::SDL_FAILURE);
    display_index = SDL_GetDisplayForWindow(handle);
    if (display_index == 0) return utility::log("Could not get display for window", utility::SDL_FAILURE);

    return SDL_APP_CONTINUE;
  }

  SDL_AppResult Window::handle_fullscreen()
  {
    if (fullscreen)
    {
      if (!SDL_SetWindowBordered(handle, true))
        return utility::log("Could not set window bordered", utility::SDL_FAILURE);
      if (!SDL_SetWindowSize(handle, starting_width, starting_height))
        return utility::log("Could not set window size", utility::SDL_FAILURE);
      if (!SDL_SetWindowPosition(handle, left, top))
        return utility::log("Could not set window position", utility::SDL_FAILURE);
    }
    else
    {
      SDL_Rect display_bounds;
      if (!SDL_GetDisplayBounds(display_index, &display_bounds))
        return utility::log("Could not get display bounds", utility::SDL_FAILURE);
      if (!SDL_SetWindowBordered(handle, false))
        return utility::log("Could not set window bordered", utility::SDL_FAILURE);
      if (!SDL_SetWindowSize(handle, display_bounds.w, display_bounds.h))
        return utility::log("Could not set window size", utility::SDL_FAILURE);
      if (!SDL_SetWindowPosition(handle, SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
                                 SDL_WINDOWPOS_CENTERED_DISPLAY(display_index)))
        return utility::log("Could not set window position", utility::SDL_FAILURE);
    }

    fullscreen = !fullscreen;
    return SDL_APP_CONTINUE;
  }
}
