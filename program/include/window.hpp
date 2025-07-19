#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"

namespace cse
{
  class Window
  {
  private:
    struct Position_color_vertex
    {
      float x = 0.0f, y = 0.0f, z = 0.0f;
      Uint8 r = 0, g = 0, b = 0, a = 0;
    };

  public:
    static std::unique_ptr<Window> create(const std::string &i_title, int i_width, int i_height, bool i_fullscreen,
                                          bool i_vsync);
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
    Window(const std::string &i_title, int i_width, int i_height, bool i_fullscreen, bool i_vsync);

    void handle_quit();
    void handle_move();
    void handle_fullscreen();
    void handle_vsync();

  private:
    const std::string title;
    int width = 0;
    int height = 0;
    bool fullscreen = false;
    bool vsync = true;
    SDL_DisplayID display_index = 0;
    int left = 0;
    int top = 0;
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

    SDL_Window *window = nullptr;
    SDL_GPUDevice *gpu = nullptr;
    SDL_GPUGraphicsPipeline *pipeline = nullptr;
    SDL_GPUBuffer *vertex_buffer = nullptr;
    SDL_GPUBuffer *index_buffer = nullptr;
    inline static std::atomic<bool> initialized = false;
  };
}
