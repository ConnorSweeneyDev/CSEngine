#pragma once

#include <functional>
#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

namespace cse::core
{
  class window
  {
    friend class game;

  private:
    struct frame
    {
      frame(const std::string &title_, const unsigned int starting_width_, const unsigned int starting_height_,
            const bool fullscreen_);

      void handle_move(SDL_Window *instance);
      void enable_fullscreen(SDL_Window *instance);
      void disable_fullscreen(SDL_Window *instance);

      const std::string title = "";
      const unsigned int starting_width = 0;
      const unsigned int starting_height = 0;
      unsigned int width = 0;
      unsigned int height = 0;
      int left = 0;
      int top = 0;
      SDL_DisplayID display_index = 0;
      bool fullscreen = false;
    };
    struct graphics
    {
      graphics(const bool vsync_);

      void enable_vsync(const std::string &title);
      void disable_vsync(const std::string &title);

      bool vsync = false;
      SDL_Window *instance = nullptr;
      SDL_GPUDevice *gpu = nullptr;
      SDL_GPUCommandBuffer *command_buffer = nullptr;
      SDL_GPURenderPass *render_pass = nullptr;
    };

  public:
    window(const std::string &title_, const unsigned int starting_width_, const unsigned int starting_height_,
           const bool fullscreen_, const bool vsync_);
    virtual ~window();
    window(const window &) = delete;
    window &operator=(const window &) = delete;
    window(window &&) = delete;
    window &operator=(window &&) = delete;

  protected:
    void toggle_fullscreen();
    void toggle_vsync();

  private:
    void initialize();
    void cleanup();
    void input();
    bool start_render();
    void end_render();

  protected:
    std::function<void(const SDL_KeyboardEvent &key)> handle_input = nullptr;

    bool running = false;

  private:
    frame frame = {"", 0, 0, false};
    graphics graphics = {false};
    const bool *key_state = nullptr;
  };
}
