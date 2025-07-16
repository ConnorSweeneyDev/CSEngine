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

    void update_time();
    bool is_behind();
    int input();
    void catchup();
    void update_alpha();
    int render();
    void update_fps();

    int handle_quit();
    int handle_move();
    int handle_fullscreen();

  public:
    float current_red = 0.1f;
    float previous_red = current_red;
    float interpolated_red = current_red;
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

    const double fixed_timestep = 1.0 / 60.0;
    double current_time = 0.0;
    double accumulator = 0.0;
    double alpha = 0.0;
    Uint64 last_fps_time = 0;
    int frame_count = 0;

    SDL_Window *handle = nullptr;
    SDL_GPUDevice *gpu = nullptr;
  };
}
