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
    static std::unique_ptr<Window> create(const std::string &i_title, bool i_fullscreen, int i_width, int i_height);
    ~Window();

    void input();
    void simulate();
    void render();

    void update_simulation_time();
    bool simulation_behind();
    void catchup_simulation();
    void update_simulation_alpha();
    bool render_behind();
    void update_fps();

  public:
    float current_strength = 0.1f;
    float previous_strength = current_strength;
    float interpolated_strength = current_strength;
    float strength_velocity = 0.0f;
    float strength_acceleration = 0.0f;

    bool running = false;

  private:
    Window(const std::string &i_title, bool i_fullscreen, int i_width, int i_height);

    void handle_quit();
    void handle_move();
    void handle_fullscreen();

  private:
    const std::string title;
    bool fullscreen = false;
    int width = 0;
    int height = 0;
    const int starting_width = 0;
    const int starting_height = 0;
    SDL_DisplayID display_index = 0;
    int left = 0;
    int top = 0;

    const double target_simulation_time = 1.0 / 60.0;
    double last_simulation_time = 0.0;
    double simulation_accumulator = 0.0;
    double simulation_alpha = 0.0;
    const double target_render_time = 1.0 / 144.0;
    double last_render_time = 0.0;
    double last_fps_time = 0;
    int frame_count = 0;

    SDL_Window *window = nullptr;
    SDL_GPUDevice *gpu = nullptr;
    SDL_GPUGraphicsPipeline *pipeline = nullptr;
    inline static std::atomic<bool> initialized = false;
  };
}
