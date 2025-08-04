#pragma once

#include <functional>
#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

namespace cse::base
{
  class window
  {
  private:
    struct graphics
    {
      int width = 0;
      int height = 0;
      int left = 0;
      int top = 0;
      SDL_DisplayID display_index = 0;
      SDL_Window *instance = nullptr;
      SDL_GPUDevice *gpu = nullptr;
      SDL_GPUCommandBuffer *command_buffer = nullptr;
      SDL_GPURenderPass *render_pass = nullptr;
    };

  public:
    window(const std::string &title_, int starting_width_, int starting_height_, bool fullscreen_, bool vsync_);
    virtual ~window();
    window(const window &) = delete;
    window &operator=(const window &) = delete;
    window(window &&) = delete;
    window &operator=(window &&) = delete;

    bool is_running() const;
    const bool *get_key_state() const;
    auto get_graphics() -> graphics const;

    void initialize();
    void cleanup();
    void input();
    bool start_render();
    void end_render();

  protected:
    void quit();
    void move();
    void toggle_fullscreen();
    void toggle_vsync();

  protected:
    std::function<void(const SDL_KeyboardEvent &key)> handle_input = nullptr;

  private:
    std::string title = "CSE Window";
    int starting_width = 1280;
    int starting_height = 720;
    bool fullscreen = false;
    bool vsync = true;
    bool running = false;
    const bool *key_state = nullptr;
    graphics graphics = {};
  };
}
