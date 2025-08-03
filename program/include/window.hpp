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
  public:
    window(const std::string &i_title, int i_starting_width, int i_starting_height, bool i_fullscreen, bool i_vsync);
    virtual ~window();

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

  public:
    bool running = false;
    int width = 0;
    int height = 0;
    SDL_Window *instance = nullptr;
    SDL_GPUDevice *gpu = nullptr;
    SDL_GPUCommandBuffer *command_buffer = nullptr;
    SDL_GPURenderPass *render_pass = nullptr;
    const bool *key_state = nullptr;

  protected:
    std::function<void(const SDL_KeyboardEvent &key)> handle_input = nullptr;

  private:
    std::string title = "CSE Window";
    int starting_width = 1280;
    int starting_height = 720;
    bool fullscreen = false;
    bool vsync = true;
    SDL_DisplayID display_index = 0;
    int left = 0;
    int top = 0;
  };
}
