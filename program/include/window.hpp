#pragma once

#include <string>

#include "SDL3/SDL_video.h"

namespace cse
{
  class Window
  {
  public:
    Window(const std::string &title, int i_width, int i_height);
    ~Window();

    bool handle_move();
    bool handle_fullscreen();

  public:
    bool initialized = false;

  private:
    bool fullscreen = false;
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;
    const int starting_width = 0;
    const int starting_height = 0;

    SDL_Window *handle = nullptr;
    SDL_DisplayID display_index = 0;
  };
}
