#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_double4.hpp"
#include "glm/ext/vector_int2.hpp"

#include "core.hpp"
#include "name.hpp"
#include "property.hpp"
#include "resource.hpp"
#include "temporal.hpp"

namespace cse::help
{
  struct game_graphics
  {
    friend class cse::game;

    struct previous
    {
      double frame{};
      temporal<double> aspect{};
      temporal<glm::dvec4> clear{};
    };
    struct active
    {
      double frame{};
      temporal<double> aspect{};
      temporal<glm::dvec4> clear{};
    };

  public:
    game_graphics() = default;
    game_graphics(const double frame_, const double aspect_, const glm::dvec4 &clear_);
    ~game_graphics() = default;
    game_graphics(const game_graphics &) = delete;
    game_graphics &operator=(const game_graphics &) = delete;
    game_graphics(game_graphics &&) = delete;
    game_graphics &operator=(game_graphics &&) = delete;

  private:
    void update_previous();

    void create_app();
    void destroy_app();

  public:
    game_graphics::previous previous{};
    game_graphics::active active{};

  private:
    double actual_frame{1.0 / active.frame};
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
      bool fullscreen{};
      bool vsync{};
    };
    struct active
    {
      property<std::string> title{};
      property<bool> fullscreen{};
      property<bool> vsync{};
    };

  public:
    window_graphics() = default;
    window_graphics(const std::string &title_, const bool fullscreen_, const bool vsync_);
    ~window_graphics() = default;
    window_graphics(const window_graphics &) = delete;
    window_graphics &operator=(const window_graphics &) = delete;
    window_graphics(window_graphics &&) = delete;
    window_graphics &operator=(window_graphics &&) = delete;

  private:
    void update_previous();

    void create_window(
      SDL_DisplayID &display, int &left, int &top, const unsigned int width, const unsigned int height,
      int &windowed_left, int &windowed_top, unsigned int &windowed_width, unsigned int &windowed_height,
      const std::function<glm::ivec2(const SDL_DisplayID display, const unsigned int width, const unsigned int height)>
        &calculate_display_center,
      const std::function<glm::ivec2(const SDL_DisplayID display, const int left, const int top)> &relative_to_absolute,
      const std::function<glm::ivec2(const SDL_DisplayID display, const int left, const int top)> &absolute_to_relative,
      const SDL_DisplayID PRIMARY, const int CENTER);
    static void generate_depth_texture(const unsigned int width, const unsigned int height, SDL_GPUDevice *gpu,
                                       SDL_GPUTexture *&depth_texture);
    bool acquire_swapchain_texture();
    void start_render_pass(const unsigned int width, const unsigned int height, const glm::dvec4 &previous_clear,
                           const glm::dvec4 &active_clear, const double previous_aspect, const double active_aspect,
                           const double alpha);
    void end_render_pass();
    void handle_title_change();
    void handle_fullscreen(const SDL_DisplayID display, const int windowed_left, const int windowed_top,
                           const unsigned int windowed_width, const unsigned int windowed_height,
                           const std::function<glm::ivec2(const SDL_DisplayID display, const unsigned int width,
                                                          const unsigned int height)> &calculate_display_center,
                           const std::function<glm::ivec2(const SDL_DisplayID display, const int left, const int top)>
                             &relative_to_absolute);
    void handle_vsync();
    void destroy_window();

  public:
    window_graphics::previous previous{};
    window_graphics::active active{};

  private:
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
    void generate_order(const camera *camera, const std::vector<std::shared_ptr<object>> &objects, const double alpha);

  private:
    std::vector<object *> order{};
  };

  struct camera_graphics
  {
    friend class cse::camera;

  private:
    struct clip
    {
      double near{};
      double far{};
    };

    struct previous
    {
      temporal<double> fov{};
      camera_graphics::clip clip{};
    };
    struct active
    {
      temporal<double> fov{};
      camera_graphics::clip clip{};
    };

  public:
    camera_graphics() = default;
    camera_graphics(const double fov_, const clip &clip_);
    ~camera_graphics() = default;
    camera_graphics(const camera_graphics &) = delete;
    camera_graphics &operator=(const camera_graphics &) = delete;
    camera_graphics(camera_graphics &&) = delete;
    camera_graphics &operator=(camera_graphics &&) = delete;

  private:
    void update_previous();

    glm::dmat4 calculate_projection_matrix(const double previous_aspect, const double active_aspect,
                                           const double alpha);

  public:
    camera_graphics::previous previous{};
    camera_graphics::active active{};
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
      float r{}, g{}, b{}, a{};
      float u{}, v{};
    };

    struct shader
    {
      cse::property<cse::vertex> vertex{};
      cse::property<cse::fragment> fragment{};
    };
    struct texture
    {
      cse::property<cse::image> image{};
      cse::animation animation{};
    };
    struct render
    {
      cse::playback playback{};
      cse::flip flip{};
      temporal<cse::color> color{};
      temporal<cse::transparency> transparency{};
    };

    struct previous
    {
      object_graphics::shader shader{};
      object_graphics::texture texture{};
      object_graphics::render render{};
      int priority{};
    };
    struct active
    {
      object_graphics::shader shader{};
      object_graphics::texture texture{};
      object_graphics::render render{};
      int priority{};
    };

  public:
    object_graphics() = default;
    object_graphics(const shader &shader_, const texture &texture_, const render &render_, const int priority_);
    ~object_graphics() = default;
    object_graphics(const object_graphics &) = delete;
    object_graphics &operator=(const object_graphics &) = delete;
    object_graphics(object_graphics &&) = delete;
    object_graphics &operator=(object_graphics &&) = delete;

  private:
    void update_previous();

    void create_pipeline_and_buffers(const name &name, SDL_Window *instance, SDL_GPUDevice *gpu);
    void upload_static_buffers(const name &name, SDL_GPUDevice *gpu);
    void upload_dynamic_buffers(const name &name, SDL_GPUDevice *gpu, const double alpha);
    void generate_pipeline();
    void generate_and_upload_texture();
    void animate(const double tick);
    void bind_pipeline_and_buffers(SDL_GPURenderPass *render_pass, const double alpha);
    void push_uniform_data(SDL_GPUCommandBuffer *command_buffer, const std::array<glm::dmat4, 3> &matrices,
                           const double alpha);
    void draw_primitives(SDL_GPURenderPass *render_pass);
    void destroy_resources(SDL_GPUDevice *gpu);

  public:
    object_graphics::previous previous{};
    object_graphics::active active{};

  private:
    SDL_Window *cached_instance{};
    SDL_GPUDevice *cached_gpu{};
    object_graphics::pipelines pipelines{};
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
