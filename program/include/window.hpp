#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

namespace cse
{
  class Window
  {
  public:
    static std::unique_ptr<Window> create(const std::string &title, bool i_fullscreen, int i_width, int i_height);
    ~Window();

    int input();
    void simulate();
    int render();

    int handle_quit();
    int handle_move();
    int handle_fullscreen();

    void update_simulation_time();
    bool simulation_behind();
    void catchup_simulation();
    void update_simulation_alpha();
    bool render_behind();
    void update_fps();

  public:
    float current_red = 0.1f;
    float previous_red = current_red;
    float interpolated_red = current_red;
    float red_velocity = 0.0f;
    float red_acceleration = 0.0f;

    bool running = false;

  private:
    Window(const std::string &title, bool i_fullscreen, int i_width, int i_height);

  private:
    bool fullscreen = false;
    SDL_DisplayID display_index = 0;
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;
    const int starting_width = 0;
    const int starting_height = 0;

    const double target_simulation_time = 1.0 / 60.0;
    double last_simulation_time = 0.0;
    double simulation_accumulator = 0.0;
    double simulation_alpha = 0.0;
    const double target_render_time = 1.0 / 144.0;
    double last_render_time = 0.0;
    double last_fps_time = 0;
    int frame_count = 0;

    SDL_Window *handle = nullptr;
    SDL_GPUDevice *gpu = nullptr;
    inline static std::atomic<bool> initialized = false;
  };
}
