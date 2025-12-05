#pragma once

#include <array>
#include <string>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_uint4_sized.hpp"

#include "declaration.hpp"
#include "property.hpp"
#include "resource.hpp"

namespace cse::helper
{
  struct window_graphics
  {
    friend class core::game;
    friend class core::window;

  public:
    window_graphics() = default;
    window_graphics(const std::string &title_, const unsigned int width_, const unsigned int height_,
                    const bool fullscreen_, const bool vsync_);
    ~window_graphics();
    window_graphics(const window_graphics &) = delete;
    window_graphics &operator=(const window_graphics &) = delete;
    window_graphics(window_graphics &&) = delete;
    window_graphics &operator=(window_graphics &&) = delete;

  private:
    void create_app_and_window();
    void generate_depth_texture();
    bool acquire_swapchain_texture();
    void start_render_pass(const float target_aspect_ratio);
    void end_render_pass();
    void handle_move();
    void handle_resize();
    void destroy_window_and_app();

  public:
    property<bool> fullscreen = {};
    property<bool> vsync = {};

  private:
    const std::string title = {};
    unsigned int width = {};
    unsigned int height = {};
    unsigned int windowed_width = {};
    unsigned int windowed_height = {};
    int left = {};
    int top = {};
    SDL_DisplayID display_index = {};
    SDL_Window *instance = {};
    SDL_GPUDevice *gpu = {};
    SDL_GPUCommandBuffer *command_buffer = {};
    SDL_GPUTexture *swapchain_texture = {};
    SDL_GPUTexture *depth_texture = {};
    SDL_GPURenderPass *render_pass = {};
  };

  struct camera_graphics
  {
    friend class core::camera;

  public:
    camera_graphics() = default;
    camera_graphics(const float fov_);

  private:
    glm::mat4 calculate_projection_matrix(const float target_aspect_ratio);

  public:
    float fov = {};

  private:
    float near_clip = {};
    float far_clip = {};
  };

  struct object_graphics
  {
    friend class core::object;

  private:
    struct vertex
    {
      float x = {}, y = {}, z = {};
      Uint8 r = {}, g = {}, b = {}, a = {};
      float u = {}, v = {};
    };
    struct shader
    {
      const resource::compiled_shader vertex = {};
      const resource::compiled_shader fragment = {};
    };
    struct texture
    {
      const resource::compiled_texture data = {};
      std::string frame_group = {};
      unsigned int frame_index = {};
    };

  public:
    object_graphics() = default;
    object_graphics(const resource::compiled_shader &vertex_shader_, const resource::compiled_shader &fragment_shader_,
                    const resource::compiled_texture &texture_, const std::string &frame_group_,
                    const glm::u8vec4 &tint_);

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
    shader shader = {};
    texture texture = {};
    glm::u8vec4 tint = {};

  private:
    SDL_GPUGraphicsPipeline *pipeline = {};
    SDL_GPUBuffer *vertex_buffer = {};
    SDL_GPUBuffer *index_buffer = {};
    SDL_GPUTexture *texture_buffer = {};
    SDL_GPUSampler *sampler_buffer = {};
    SDL_GPUTransferBuffer *vertex_transfer_buffer = {};
    SDL_GPUTransferBuffer *texture_transfer_buffer = {};
    std::array<vertex, 4> quad_vertices = {};
    std::array<Uint16, 6> quad_indices = {};
  };
}
