#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_double4.hpp"
#include "glm/ext/vector_int2.hpp"

#include "core.hpp"
#include "resource.hpp"
#include "temporal.hpp"

namespace cse::help
{
  struct game_graphics
  {
    friend class cse::game;
    friend struct scene_graphics;

  private:
    struct frame
    {
      double target{};
      unsigned int count{};
      double average{};
    };

    struct corner
    {
      float x{}, y{};
      float u{}, v{};
    };
    struct buffer
    {
      SDL_GPUBuffer *vertex{};
      SDL_GPUBuffer *index{};
      SDL_GPUSampler *sample{};
    };
    struct pipeline
    {
      SDL_GPUGraphicsPipeline *opaque{};
      SDL_GPUGraphicsPipeline *transparent{};
      SDL_GPUGraphicsPipeline *interface{};
    };
    struct object
    {
      struct batch
      {
        std::size_t first{};
        std::size_t count{};
        SDL_GPUGraphicsPipeline *pipeline{};
        SDL_GPUTexture *texture{};
      };
      struct sample
      {
        std::array<float, 16> model{};
        float red{}, green{}, blue{}, alpha{};
        float left{}, bottom{}, right{}, top{};
        float transparency{};
      };
      std::vector<batch> batches{};
      std::vector<sample> samples{};
      std::size_t capacity{};
      SDL_GPUBuffer *buffer{};
      SDL_GPUTransferBuffer *transfer_buffer{};
    };
    struct interface
    {
      struct label
      {
        std::string text{};
        const unsigned char *font{};
        unsigned int size{};
        TTF_FontStyleFlags style{};
        glm::dvec4 color{};
        TTF_HorizontalAlignment align{};
        bool wrap{};
        unsigned int width{};
        int skip{};
        SDL_GPUTexture *texture{};
        unsigned int texture_width{};
        unsigned int texture_height{};
        std::uint64_t stamp{};
      };
      std::vector<cse::interface *> order{};
      std::map<const cse::interface *, label> labels{};
      std::uint64_t stamp{};
    };
    struct cache
    {
      using pipeline_key = std::tuple<const unsigned char *, std::size_t, const unsigned char *, std::size_t>;
      using texture_key = std::pair<const unsigned char *, std::size_t>;
      using font_key = std::tuple<const unsigned char *, std::size_t, unsigned int>;
      struct typeface
      {
        TTF_Font *font{};
        int skip{};
      };
      template <typename handle> struct cached
      {
        handle value{};
        std::uint64_t stamp{};
      };
      std::map<pipeline_key, cached<game_graphics::pipeline>> pipeline{};
      std::map<texture_key, cached<SDL_GPUTexture *>> texture{};
      std::map<font_key, cached<typeface>> font{};
      std::uint64_t stamp{};
    };

    struct previous
    {
      game_graphics::frame frame{};
      temporal<double> aspect{};
      unsigned int resolution{};
      temporal<glm::dvec4> clear{};
    };
    struct active
    {
      game_graphics::frame frame{};
      temporal<double> aspect{};
      unsigned int resolution{};
      temporal<glm::dvec4> clear{};
    };

  public:
    game_graphics() = default;
    game_graphics(const double frame_, const double aspect_, const unsigned int resolution_, const glm::dvec4 &clear_);
    ~game_graphics() = default;
    game_graphics(const game_graphics &) = delete;
    game_graphics &operator=(const game_graphics &) = delete;
    game_graphics(game_graphics &&) = delete;
    game_graphics &operator=(game_graphics &&) = delete;

  private:
    void update_previous();

    void create_app();
    void create(SDL_GPUDevice *gpu);
    void render(const std::vector<std::shared_ptr<cse::interface>> &scene_interfaces,
                const std::vector<std::shared_ptr<cse::interface>> &game_interfaces, SDL_Window *instance,
                SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                const double alpha);
    void destroy(SDL_GPUDevice *gpu);
    void destroy_app();

    void generate_order(const std::vector<std::shared_ptr<cse::interface>> &scene_interfaces,
                        const std::vector<std::shared_ptr<cse::interface>> &game_interfaces);
    void generate_samples_and_batches(const std::vector<cse::object *> &order, SDL_Window *instance, SDL_GPUDevice *gpu,
                                      const double alpha);
    void generate_samples_and_batches(const std::vector<cse::interface *> &order, SDL_Window *instance,
                                      SDL_GPUDevice *gpu, const double alpha);
    void upload_samples(SDL_GPUDevice *gpu);
    void draw_batches(const std::pair<glm::dmat4, glm::dmat4> &matrices, SDL_GPUCommandBuffer *command_buffer,
                      SDL_GPURenderPass *render_pass);
    pipeline &require_pipelines(SDL_Window *instance, SDL_GPUDevice *gpu, const cse::vertex &vertex,
                                const cse::fragment &fragment);
    SDL_GPUTexture *require_texture(SDL_GPUDevice *gpu, const cse::image &image);
    cache::typeface &require_font(const cse::font &font, const unsigned int size);
    game_graphics::interface::label &require_label(SDL_GPUDevice *gpu, const cse::interface *element,
                                                   const double alpha);
    std::pair<glm::dmat4, glm::dmat4> calculate_interface_matrices(const double alpha) const;

  public:
    game_graphics::previous previous{};
    game_graphics::active active{};

  private:
    double actual_frame{1.0 / active.frame.target};
    game_graphics::buffer buffer{};
    game_graphics::cache cache{};
    game_graphics::object object{};
    game_graphics::interface interface{};
  };

  struct window_graphics
  {
    friend class cse::game;
    friend class cse::window;
    friend class cse::scene;

  private:
    struct shadow
    {
      SDL_DisplayID display{};
      int left{};
      int top{};
      unsigned int width{};
      unsigned int height{};
      std::string title{};
      bool fullscreen{};
      bool vsync{};
    };

    struct previous
    {
      std::string title{};
      bool fullscreen{};
      bool vsync{};
    };
    struct active
    {
      std::string title{};
      bool fullscreen{};
      bool vsync{};
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

    void create_window(SDL_DisplayID &display, int &left, int &top, const unsigned int width, const unsigned int height,
                       const SDL_DisplayID PRIMARY, const int CENTER);
    void generate_depth_texture(const unsigned int width, const unsigned int height);
    bool acquire_swapchain_texture();
    void start_render_pass(const unsigned int width, const unsigned int height, const glm::dvec4 &previous_clear,
                           const glm::dvec4 &active_clear, const double previous_aspect, const double active_aspect,
                           const double alpha);
    void end_render_pass();
    void destroy_window();

    void reconcile(SDL_DisplayID &display, int &left, int &top, const unsigned int width, const unsigned int height,
                   const SDL_DisplayID PRIMARY, const int CENTER);
    void handle_move(SDL_DisplayID &display, int &left, int &top);
    void handle_resize(SDL_DisplayID &display, int &left, int &top, unsigned int &width, unsigned int &height);
    void handle_manual_move(SDL_DisplayID &display, int &left, int &top, const unsigned int width,
                            const unsigned int height, const int CENTER);
    void handle_manual_display_move(SDL_DisplayID &display, int &left, int &top, const unsigned int width,
                                    const unsigned int height, const SDL_DisplayID PRIMARY);
    void handle_manual_resize(SDL_DisplayID &display, int &left, int &top, const unsigned int width,
                              const unsigned int height);
    void handle_title_change();
    void handle_fullscreen(const SDL_DisplayID display);
    void handle_vsync();
    glm::ivec2 calculate_display_center(const SDL_DisplayID display, const unsigned int width,
                                        const unsigned int height);
    glm::ivec2 relative_to_absolute(const SDL_DisplayID display, const int left, const int top);
    glm::ivec2 absolute_to_relative(const SDL_DisplayID display, const int left, const int top);

  public:
    window_graphics::previous previous{};
    window_graphics::active active{};

  private:
    window_graphics::shadow shadow{};
    SDL_Window *instance{};
    SDL_GPUDevice *gpu{};
    SDL_GPUCommandBuffer *command_buffer{};
    SDL_GPUTexture *swapchain_texture{};
    SDL_GPUTexture *depth_texture{};
    SDL_GPURenderPass *render_pass{};
    int windowed_left{};
    int windowed_top{};
    unsigned int windowed_width{};
    unsigned int windowed_height{};
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
    void render(SDL_Window *instance, SDL_GPUDevice *gpu, game_graphics &graphics, const camera *camera,
                const std::vector<std::shared_ptr<object>> &objects, const std::pair<glm::dmat4, glm::dmat4> &matrices,
                SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass, const double alpha);

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
    struct shader
    {
      cse::vertex vertex{};
      cse::fragment fragment{};
    };
    struct texture
    {
      cse::image image{};
      cse::animation animation{};
      cse::playback playback{};
      cse::flip flip{};
      temporal<cse::color> color{};
      temporal<cse::transparency> transparency{};
    };

    struct previous
    {
      object_graphics::shader shader{};
      object_graphics::texture texture{};
      int priority{};
    };
    struct active
    {
      object_graphics::shader shader{};
      object_graphics::texture texture{};
      int priority{};
    };

  public:
    object_graphics() = default;
    object_graphics(const shader &shader_, const texture &texture_, const int priority_);
    ~object_graphics() = default;
    object_graphics(const object_graphics &) = delete;
    object_graphics &operator=(const object_graphics &) = delete;
    object_graphics(object_graphics &&) = delete;
    object_graphics &operator=(object_graphics &&) = delete;

  private:
    void update_previous();

    void animate(const double tick);

  public:
    object_graphics::previous previous{};
    object_graphics::active active{};
  };

  struct interface_graphics
  {
    friend class cse::interface;

  private:
    struct shader
    {
      cse::vertex vertex{};
      cse::fragment fragment{};
    };
    struct texture
    {
      cse::image image{};
      cse::animation animation{};
      cse::playback playback{};
      cse::flip flip{};
      temporal<cse::color> color{};
      temporal<cse::transparency> transparency{};
    };
    struct text
    {
      std::string content{};
      cse::font font{};
      unsigned int size{};
      cse::style style{};
      temporal<cse::color> color{};
      cse::align align{};
      double spacing{};
      bool wrap{};
      bool overflow{};
    };

    struct previous
    {
      interface_graphics::shader shader{};
      interface_graphics::texture texture{};
      interface_graphics::text text{};
      int priority{};
    };
    struct active
    {
      interface_graphics::shader shader{};
      interface_graphics::texture texture{};
      interface_graphics::text text{};
      int priority{};
    };

  public:
    interface_graphics() = default;
    interface_graphics(const shader &shader_, const texture &texture_, const text &text_, const int priority_);
    ~interface_graphics() = default;
    interface_graphics(const interface_graphics &) = delete;
    interface_graphics &operator=(const interface_graphics &) = delete;
    interface_graphics(interface_graphics &&) = delete;
    interface_graphics &operator=(interface_graphics &&) = delete;

  private:
    void update_previous();

    void animate(const double tick);

  public:
    interface_graphics::previous previous{};
    interface_graphics::active active{};
  };
}
