#pragma once

#include <string>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"

namespace cse
{
  class Window
  {
  public:
    Window(const std::string &title, int i_width, int i_height);
    ~Window();

    void update_delta_time();
    int handle_events();
    int update();
    void catchup();

    int handle_quit();
    int handle_move();
    int handle_fullscreen();

  public:
    float red = 0.1f;
    bool running = false;

  private:
    bool fullscreen = false;
    SDL_DisplayID display_index = 0;
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;
    const int starting_width = 0;
    const int starting_height = 0;

    Uint64 target_frame_rate = 144;
    Uint64 accumulated_frames = 0;
    Uint64 current_frame_time = 0;
    Uint64 last_frame_time = 0;
    Uint64 past_frame_time = 0;
    double delta_time = 0.0;

    SDL_Window *handle = nullptr;
    SDL_GPUDevice *gpu = nullptr;
  };
}
