#pragma once

#include <array>
#include <string>
#include <utility>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "glm/ext/vector_uint4_sized.hpp"

#include "declaration.hpp"
#include "resource.hpp"

namespace cse::help
{
  struct window_graphics
  {
    friend class cse::game;
    friend class cse::window;
    friend class cse::scene;

  public:
    window_graphics() = default;
    ~window_graphics();
    window_graphics(const window_graphics &) = delete;
    window_graphics &operator=(const window_graphics &) = delete;
    window_graphics(window_graphics &&) = delete;
    window_graphics &operator=(window_graphics &&) = delete;

  private:
    void create_app_and_window(const std::string &title, const unsigned int width, const unsigned int height, int &left,
                               int &top, SDL_DisplayID &display_index, const bool fullscreen, const bool vsync);
    bool acquire_swapchain_texture();
    void start_render_pass(const float target_aspect_ratio, const unsigned int width, const unsigned int height);
    void end_render_pass();
    void generate_depth_texture(const unsigned int width, const unsigned int height);
    glm::uvec2 calculate_display_center(const SDL_DisplayID display_index, const unsigned int width,
                                        const unsigned int height);
    void handle_move(int &left, int &top, SDL_DisplayID &display_index, const bool fullscreen);
    void handle_manual_move(const int left, const int top, const bool fullscreen);
    void handle_manual_display_move(const unsigned int width, const unsigned int height, int &left, int &top,
                                    const SDL_DisplayID display_index, const bool fullscreen);
    void handle_resize(unsigned int &width, unsigned int &height, SDL_DisplayID &display_index, const bool fullscreen);
    void handle_manual_resize(const unsigned int width, const unsigned int height, const bool fullscreen);
    void handle_fullscreen(const bool fullscreen, int &left, int &top, const SDL_DisplayID display_index);
    void handle_vsync(const bool vsync);
    void destroy_window_and_app();

  private:
    unsigned int windowed_width{};
    unsigned int windowed_height{};
    int windowed_left{};
    int windowed_top{};
    SDL_Window *instance{};
    SDL_GPUDevice *gpu{};
    SDL_GPUCommandBuffer *command_buffer{};
    SDL_GPUTexture *swapchain_texture{};
    SDL_GPUTexture *depth_texture{};
    SDL_GPURenderPass *render_pass{};
  };

  struct camera_graphics
  {
    friend class cse::camera;

  public:
    camera_graphics() = default;
    camera_graphics(const float fov_);

  private:
    glm::mat4 calculate_projection_matrix(const float target_aspect_ratio);

  public:
    float fov{};

  private:
    float near_clip{};
    float far_clip{};
  };

  struct object_graphics
  {
    friend class cse::object;

  private:
    struct vertex
    {
      float x{}, y{}, z{};
      Uint8 r{}, g{}, b{}, a{};
      float u{}, v{};
    };
    struct shader
    {
      const cse::shader vertex{};
      const cse::shader fragment{};
    };
    struct texture
    {
      const image image{};
      frame_group group{};
      unsigned int frame{};
    };

  public:
    object_graphics() = default;
    object_graphics(const glm::u8vec4 &color_, const std::pair<cse::shader, cse::shader> &shader_,
                    const std::pair<cse::image, cse::frame_group> &texture_);
    ~object_graphics();
    object_graphics(const object_graphics &) = delete;
    object_graphics &operator=(const object_graphics &) = delete;
    object_graphics(object_graphics &&) = delete;
    object_graphics &operator=(object_graphics &&) = delete;

  private:
    void create_pipeline_and_buffers(SDL_Window *instance, SDL_GPUDevice *gpu);
    void upload_static_buffers(SDL_GPUDevice *gpu);
    void upload_dynamic_buffers(SDL_GPUDevice *gpu);
    void bind_pipeline_and_buffers(SDL_GPURenderPass *render_pass);
    void push_uniform_data(SDL_GPUCommandBuffer *command_buffer, const glm::mat4 &projection_matrix,
                           const glm::mat4 &view_matrix, const glm::mat4 &model_matrix);
    void draw_primitives(SDL_GPURenderPass *render_pass);
    void cleanup_object(SDL_GPUDevice *gpu);

  public:
    glm::u8vec4 color{};
    shader shader{};
    texture texture{};

  private:
    SDL_GPUGraphicsPipeline *pipeline{};
    SDL_GPUBuffer *vertex_buffer{};
    SDL_GPUBuffer *index_buffer{};
    SDL_GPUTexture *texture_buffer{};
    SDL_GPUSampler *sampler_buffer{};
    SDL_GPUTransferBuffer *vertex_transfer_buffer{};
    SDL_GPUTransferBuffer *texture_transfer_buffer{};
    std::array<vertex, 4> quad_vertices{};
    std::array<Uint16, 6> quad_indices{};
  };
}
