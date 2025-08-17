#pragma once

#include <functional>
#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "declaration.hpp"
#include "property.hpp"

namespace cse::core
{
  class window
  {
    friend class game;

  private:
    struct graphics
    {
      friend class game;
      friend class window;

    public:
      graphics() = default;
      graphics(const std::string &title_, const unsigned int starting_width_, const unsigned int starting_height_,
               const bool fullscreen_, const bool vsync_);
      ~graphics();

    private:
      void initialize_app();
      void create_window();
      bool create_command_and_swapchain();
      void create_render_pass();
      void end_render_and_submit_command();
      void enable_fullscreen();
      void disable_fullscreen();
      void enable_vsync();
      void disable_vsync();
      void handle_move();
      void cleanup_gpu_and_instance();

    public:
      helper::property<bool> fullscreen = {};
      helper::property<bool> vsync = {};

    private:
      const std::string title = {};
      const unsigned int starting_width = {};
      const unsigned int starting_height = {};
      unsigned int width = {};
      unsigned int height = {};
      int left = {};
      int top = {};
      SDL_DisplayID display_index = {};
      SDL_Window *instance = {};
      SDL_GPUDevice *gpu = {};
      SDL_GPUCommandBuffer *command_buffer = {};
      SDL_GPUTexture *swapchain_texture = {};
      SDL_GPURenderPass *render_pass = {};
    };

  public:
    window(const std::string &title_, const unsigned int starting_width_, const unsigned int starting_height_,
           const bool fullscreen_, const bool vsync_);
    virtual ~window();
    window(const window &) = delete;
    window &operator=(const window &) = delete;
    window(window &&) = delete;
    window &operator=(window &&) = delete;

  private:
    void initialize();
    void cleanup();
    void input();
    bool start_render();
    void end_render();

  protected:
    std::function<void(const SDL_KeyboardEvent &key)> handle_event = {};
    std::function<void(const bool *key_state)> handle_input = {};

    bool running = {};
    graphics graphics = {};

  private:
    const bool *keys = {};
  };
}
