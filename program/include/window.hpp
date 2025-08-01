#pragma once

#include <string>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

namespace cse::base
{
  class window
  {
  public:
    window(const std::string &i_title, int i_starting_width, int i_starting_height, bool i_fullscreen, bool i_vsync);
    virtual ~window();

    virtual bool is_running() const;
    virtual void input();
    virtual bool start_render();
    virtual void end_render();

  private:
    void handle_quit();
    void handle_move();
    void handle_fullscreen();
    void handle_vsync();

  public:
    int width = 0;
    int height = 0;
    const bool *key_state = nullptr;
    SDL_Window *instance = nullptr;
    SDL_GPUDevice *gpu = nullptr;
    SDL_GPUCommandBuffer *command_buffer = nullptr;
    SDL_GPURenderPass *render_pass = nullptr;

  private:
    std::string title = "CSE Window";
    int starting_width = 1280;
    int starting_height = 720;
    bool fullscreen = false;
    bool vsync = true;
    bool running = false;
    SDL_DisplayID display_index = 0;
    int left = 0;
    int top = 0;
  };
}
