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
      friend class window;

    public:
      graphics(const std::string &title_, const int starting_width_, const int starting_height_);

    private:
      void initialize_window();
      void initialize_gpu();
      void show_window() const;
      void generate_command_buffer();
      bool generate_render_pass();
      void generate_viewport();
      void end_render_and_submit();
      void detect_display_index();
      void change_to_windowed();
      void change_to_fullscreen();
      void change_to_immediate();
      void change_to_vsync();
      void cleanup_window();

    private:
      const std::string title = "";
      int width = 0;
      int height = 0;
      const int starting_width = 0;
      const int starting_height = 0;
      int left = 0;
      int top = 0;
      SDL_DisplayID display_index = 0;
      SDL_Window *instance = nullptr;
      SDL_GPUDevice *gpu = nullptr;
      SDL_GPUCommandBuffer *command_buffer = nullptr;
      SDL_GPURenderPass *render_pass = nullptr;
    };

  public:
    window(const std::string &title_, const int starting_width_, const int starting_height_, const bool fullscreen_,
           const bool vsync_);
    virtual ~window();
    window(const window &) = delete;
    window &operator=(const window &) = delete;
    window(window &&) = delete;
    window &operator=(window &&) = delete;

    bool is_running() const;
    int get_width() const;
    int get_height() const;
    const bool *get_key_state() const;
    SDL_Window *get_instance() const;
    SDL_GPUDevice *get_gpu() const;
    SDL_GPUCommandBuffer *get_command_buffer() const;
    SDL_GPURenderPass *get_render_pass() const;

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
    bool fullscreen = false;
    bool vsync = false;
    bool running = false;
    const bool *key_state = nullptr;
    graphics graphics = {"", 0, 0};
  };
}
