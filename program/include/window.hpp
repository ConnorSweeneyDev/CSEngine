#pragma once

#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/ext/vector_int2.hpp"

#include "core.hpp"
#include "input.hpp"
#include "mixer.hpp"
#include "timer.hpp"

inline constexpr SDL_DisplayID PRIMARY{1000000};
inline constexpr int ORIGIN{2000000};
enum mode
{
  WINDOWED,
  BORDERLESS,
  FULLSCREEN
};

namespace cse::help::window
{
  struct previous
  {
  public:
    previous() = default;
    previous(const std::string &title_, const SDL_DisplayID display_, const int left_, const int top_,
             const unsigned int width_, const unsigned int height_, const ::mode mode_, const bool vsync_,
             const cse::mouse::initial &mouse_);
    ~previous() = default;
    previous(const previous &) = delete;
    previous &operator=(const previous &) = delete;
    previous(previous &&) = delete;
    previous &operator=(previous &&) = delete;

  public:
    std::string title{};
    SDL_DisplayID display{};
    int left{};
    int top{};
    unsigned int width{};
    unsigned int height{};
    ::mode mode{};
    bool vsync{};

    bool running{};
    cse::keyboard keyboard{};
    cse::mouse mouse{};
    help::timer timer{};
    help::mixer mixer{};
    help::phase phase{};
  };

  struct active
  {
    friend class cse::game;
    friend struct game::active;
    friend class cse::window;
    friend class cse::scene;

  private:
    struct viewport
    {
      double left{};
      double top{};
      double width{};
      double height{};
    };
    struct shadow
    {
      std::string title{};
      SDL_DisplayID display{};
      int left{};
      int top{};
      unsigned int width{};
      unsigned int height{};
      ::mode mode{};
      bool vsync{};
      cse::mouse mouse{};
    };

  public:
    active() = default;
    active(const std::string &title_, const SDL_DisplayID display_, const int left_, const int top_,
           const unsigned int width_, const unsigned int height_, const ::mode mode_, const bool vsync_,
           const cse::mouse::initial &mouse_);
    ~active() = default;
    active(const active &) = delete;
    active &operator=(const active &) = delete;
    active(active &&) = delete;
    active &operator=(active &&) = delete;

  private:
    void create(SDL_GPUDevice *device, const double aspect, const unsigned int resolution);
    void synchronize(previous &last);
    void render(const help::game::active &game_active, const double aspect, const glm::dvec3 &clear);
    void destroy(SDL_GPUDevice *device);

    void poll(const double aspect, const unsigned int resolution);
    viewport letterbox(const double aspect);
    float pixel_density();
    glm::ivec2 pixel_size();
    glm::dvec2 to_virtual(const double x, const double y, const double aspect, const unsigned int resolution);
    glm::dvec2 to_pixel(const double x, const double y, const double aspect, const unsigned int resolution);

    void reconcile(SDL_GPUDevice *device);
    void generate_depth_texture(SDL_GPUDevice *device);
    bool acquire_swapchain_texture(SDL_GPUDevice *device);
    bool can_move();
    bool display_exists(const SDL_DisplayID target);
    void handle_title_change();
    void handle_move();
    void handle_resize(SDL_GPUDevice *device);
    void handle_manual_display_move();
    void handle_manual_move();
    void handle_manual_resize(SDL_GPUDevice *device);
    void handle_mode();
    void handle_vsync(SDL_GPUDevice *device);
    glm::ivec2 calculate_display_center(const unsigned int w, const unsigned int h);
    glm::ivec2 relative_to_absolute(const int x, const int y);
    glm::ivec2 absolute_to_relative(const int x, const int y);

  public:
    std::string title{};
    SDL_DisplayID display{};
    int left{};
    int top{};
    unsigned int width{};
    unsigned int height{};
    ::mode mode{};
    bool vsync{};

    bool running{};
    cse::keyboard keyboard{};
    cse::mouse mouse{};
    help::timer timer{};
    help::mixer mixer{};
    help::phase phase{};

  private:
    active::shadow shadow{};
    SDL_Event event{};

    SDL_Window *instance{};
    SDL_GPUCommandBuffer *command_buffer{};
    SDL_GPUTexture *swapchain_texture{};
    SDL_GPUTexture *depth_texture{};
    SDL_GPURenderPass *render_pass{};
    int windowed_left{};
    int windowed_top{};
    unsigned int render_width{};
    unsigned int render_height{};
  };
}

namespace cse
{
  class window
  {
    friend class game;

  protected:
    struct initial
    {
      const std::string title{"CSEngine"};
      const SDL_DisplayID display{PRIMARY};
      const int left{ORIGIN};
      const int top{ORIGIN};
      const unsigned int width{1280};
      const unsigned int height{720};
      const ::mode mode{WINDOWED};
      const bool vsync{};
      const cse::mouse::initial mouse{};
    };

  public:
    virtual ~window() = default;
    window(const window &) = delete;
    window &operator=(const window &) = delete;
    window(window &&) = delete;
    window &operator=(window &&) = delete;

  protected:
    window(const initial &initial_);
    virtual void on_prepare();
    virtual void on_create();
    virtual void on_synchronize();
    virtual void on_event(const SDL_Event &event);
    virtual void on_simulate(const double tick);
    virtual void on_render(const double alpha);
    virtual void on_destroy();
    virtual void on_clean();

  private:
    void prepare();
    void create(SDL_GPUDevice *device, const double aspect, const unsigned int resolution);
    void synchronize();
    void event(SDL_GPUDevice *device);
    void simulate(const double tick);
    void render(const double aspect, const glm::dvec3 &clear, const double alpha);
    void destroy(SDL_GPUDevice *device);
    void clean();

    bool available(SDL_GPUDevice *device);

  public:
    cse::game *game{};
    help::window::previous previous{};
    help::window::active active{};
  };
}
