#pragma once

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "glm/ext/vector_uint4_sized.hpp"

#include "declaration.hpp"
#include "name.hpp"
#include "resource.hpp"
#include "wrapper.hpp"

namespace cse::help
{
  struct game_graphics
  {
    friend class cse::game;

    struct previous
    {
      double frame_rate{};
      double aspect_ratio{};
    };
    struct active
    {
      double frame_rate{};
      double aspect_ratio{};
    };

  public:
    game_graphics() = default;
    game_graphics(const double frame_rate_, const double aspect_ratio_);
    ~game_graphics() = default;
    game_graphics(const game_graphics &) = delete;
    game_graphics &operator=(const game_graphics &) = delete;
    game_graphics(game_graphics &&) = delete;
    game_graphics &operator=(game_graphics &&) = delete;

  public:
    void update_previous();

    void create_app();
    void destroy_app();

  public:
    struct previous previous{};
    struct active active{};

  private:
    double actual_frame_rate{1.0 / active.frame_rate};
  };

  struct window_graphics
  {
    friend class cse::game;
    friend class cse::window;
    friend class cse::scene;

  private:
    struct previous
    {
      std::string title{};
    };
    struct active
    {
      cse::property<std::string> title{};
    };

  public:
    window_graphics() = default;
    window_graphics(const std::string &title_);
    ~window_graphics();
    window_graphics(const window_graphics &) = delete;
    window_graphics &operator=(const window_graphics &) = delete;
    window_graphics(window_graphics &&) = delete;
    window_graphics &operator=(window_graphics &&) = delete;

  private:
    void update_previous();

    void create_window(const unsigned int width, const unsigned int height, int &left, int &top,
                       SDL_DisplayID &display_index, const bool fullscreen, const bool vsync);
    bool acquire_swapchain_texture();
    void start_render_pass(const unsigned int width, const unsigned int height, const float aspect_ratio);
    void end_render_pass();
    void generate_depth_texture(const unsigned int width, const unsigned int height);
    glm::uvec2 calculate_display_center(const SDL_DisplayID display_index, const unsigned int width,
                                        const unsigned int height);
    void handle_title_change();
    void handle_move(int &left, int &top, SDL_DisplayID &display_index, const bool fullscreen);
    void handle_manual_move(const int left, const int top, const bool fullscreen);
    void handle_manual_display_move(const unsigned int width, const unsigned int height, int &left, int &top,
                                    const SDL_DisplayID display_index, const bool fullscreen);
    void handle_resize(unsigned int &width, unsigned int &height, SDL_DisplayID &display_index, const bool fullscreen);
    void handle_manual_resize(const unsigned int width, const unsigned int height, const bool fullscreen);
    void handle_fullscreen(const bool fullscreen, const SDL_DisplayID display_index);
    void handle_vsync(const bool vsync);
    void destroy_window();

  public:
    struct previous previous{};
    struct active active{};

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

  struct scene_graphics
  {
    friend class cse::scene;

  public:
    scene_graphics() = default;
    ~scene_graphics() = default;
    scene_graphics(const scene_graphics &) = delete;
    scene_graphics &operator=(const scene_graphics &) = delete;
    scene_graphics(scene_graphics &&) = delete;
    scene_graphics &operator=(scene_graphics &&) = delete;

  private:
    std::vector<std::shared_ptr<object>>
    generate_render_order(const std::shared_ptr<camera> camera,
                          const std::unordered_map<help::name, std::shared_ptr<object>> &objects, const double alpha);
  };

  struct camera_graphics
  {
    friend class cse::camera;

  private:
    struct previous
    {
      double fov{};
    };
    struct active
    {
      double fov{};
    };

  public:
    camera_graphics() = default;
    camera_graphics(const double fov_);
    ~camera_graphics() = default;
    camera_graphics(const camera_graphics &) = delete;
    camera_graphics &operator=(const camera_graphics &) = delete;
    camera_graphics(camera_graphics &&) = delete;
    camera_graphics &operator=(camera_graphics &&) = delete;

  private:
    void update_previous();

    glm::mat4 calculate_projection_matrix(const double alpha, const float aspect_ratio);

  public:
    struct previous previous{};
    struct active active{};

  private:
    float near_clip{};
    float far_clip{};
  };

  struct object_graphics
  {
    friend class cse::object;

  private:
    struct pipelines
    {
      SDL_GPUGraphicsPipeline *opaque{};
      SDL_GPUGraphicsPipeline *transparent{};
    };
    struct vertex_data
    {
      float x{}, y{}, z{};
      Uint8 r{}, g{}, b{}, a{};
      float u{}, v{};
    };

    struct shader
    {
      cse::property<struct vertex> vertex{};
      cse::property<struct fragment> fragment{};
    };
    struct texture
    {
      struct flip
      {
        bool horizontal{};
        bool vertical{};
      };
      cse::property<struct image> image{};
      struct group group{};
      struct animation animation{};
      struct flip flip{};
      glm::u8vec4 color{};
      double transparency{};
    };
    struct property
    {
      int priority{};
    };

    struct previous
    {
      previous() = default;
      previous(const struct shader &shader_, const struct texture &texture_, const struct property &property_);

      struct shader shader{};
      struct texture texture{};
      struct property property{};
    };
    struct active
    {
      struct shader shader{};
      struct texture texture{};
      struct property property{};
    };

  public:
    object_graphics() = default;
    object_graphics(const struct shader &shader_, const struct texture &texture_, const struct property &property_);
    ~object_graphics();
    object_graphics(const object_graphics &) = delete;
    object_graphics &operator=(const object_graphics &) = delete;
    object_graphics(object_graphics &&) = delete;
    object_graphics &operator=(object_graphics &&) = delete;

  private:
    void update_previous();

    void create_pipeline_and_buffers(SDL_Window *instance, SDL_GPUDevice *gpu);
    void upload_static_buffers(SDL_GPUDevice *gpu);
    void upload_dynamic_buffers(SDL_GPUDevice *gpu, const double alpha);
    void generate_pipeline();
    void generate_and_upload_texture();
    void update_animation(const float poll_rate);
    void bind_pipeline_and_buffers(SDL_GPURenderPass *render_pass, const double alpha);
    void push_uniform_data(SDL_GPUCommandBuffer *command_buffer, const std::array<glm::mat4, 3> &matrices,
                           const double alpha);
    void draw_primitives(SDL_GPURenderPass *render_pass);
    void destroy_resources(SDL_GPUDevice *gpu);

  public:
    struct previous previous{};
    struct active active{};

  private:
    SDL_Window *cached_instance{};
    SDL_GPUDevice *cached_gpu{};
    struct pipelines pipelines{};
    SDL_GPUBuffer *vertex_buffer{};
    SDL_GPUBuffer *index_buffer{};
    SDL_GPUTexture *texture_buffer{};
    SDL_GPUSampler *sampler_buffer{};
    SDL_GPUTransferBuffer *vertex_transfer_buffer{};
    SDL_GPUTransferBuffer *texture_transfer_buffer{};
    std::array<vertex_data, 4> quad_vertices{};
    std::array<Uint16, 6> quad_indices{};
  };
}
