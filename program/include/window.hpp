#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <string>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/vector_float3.hpp"

namespace cse
{
  class window
  {
  public:
    static std::unique_ptr<window> create(const std::string &i_title, int i_starting_width, int i_starting_height,
                                          bool i_fullscreen, bool i_vsync);
    ~window();

    bool is_running();
    void input();
    void simulate();
    void render();

    void update_simulation_time();
    bool simulation_behind();
    void catchup_simulation();
    void update_simulation_alpha();
    bool render_behind();
    void update_fps();

  private:
    struct position_color_vertex
    {
      float x = 0.0f, y = 0.0f, z = 0.0f;
      Uint8 r = 0, g = 0, b = 0, a = 0;
    };

  private:
    window(const std::string &i_title, int i_starting_width, int i_starting_height, bool i_fullscreen, bool i_vsync);
    window(const window &) = delete;
    window &operator=(const window &) = delete;
    window(window &&) = delete;
    window &operator=(window &&) = delete;

    void handle_quit();
    void handle_move();
    void handle_fullscreen();
    void handle_vsync();

  private:
    const std::string title;
    const int starting_width = 0;
    const int starting_height = 0;
    int width = 0;
    int height = 0;
    bool fullscreen = false;
    bool vsync = true;
    bool running = false;
    SDL_DisplayID display_index = 0;
    int left = 0;
    int top = 0;

    SDL_Window *instance = nullptr;
    inline static std::atomic<bool> initialized = false;

    const double target_simulation_time = 1.0 / 60.0;
    double last_simulation_time = 0.0;
    double simulation_accumulator = 0.0;
    double simulation_alpha = 0.0;
    const double target_render_time = 1.0 / 144.0;
    double last_render_time = 0.0;
    double last_fps_time = 0;
    int frame_count = 0;

    glm::vec3 current_view_translation = glm::vec3(0.0f, 0.0f, 2.0f);
    glm::vec3 previous_view_translation = current_view_translation;
    glm::vec3 interpolated_view_translation = current_view_translation;
    glm::vec3 view_translation_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 view_translation_acceleration = glm::vec3(0.0f, 0.0f, 0.0f);

    SDL_GPUDevice *gpu = nullptr;
    SDL_GPUGraphicsPipeline *pipeline = nullptr;
    SDL_GPUBuffer *vertex_buffer = nullptr;
    SDL_GPUBuffer *index_buffer = nullptr;
    const std::array<position_color_vertex, 4> default_quad_vertices = {
      position_color_vertex{0.5f, 0.5f, 0.0f, 0, 0, 255, 255},
      position_color_vertex{0.5f, -0.5f, 0.0f, 0, 255, 0, 255},
      position_color_vertex{-0.5f, 0.5f, 0.0f, 255, 255, 255, 255},
      position_color_vertex{-0.5f, -0.5f, 0.0f, 255, 0, 0, 255},
    };
    const std::array<Uint16, 6> default_quad_indices = {3, 1, 0, 3, 0, 2};
  };
}
