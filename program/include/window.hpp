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

    virtual void initialize() = 0;
    virtual void cleanup() = 0;

    virtual void input() = 0;
    virtual bool start_render() = 0;
    virtual void end_render() = 0;

  protected:
    void handle_quit();
    void handle_move();
    void handle_fullscreen();
    void handle_vsync();

  public:
    bool running = false;
    int width = 0;
    int height = 0;
    const bool *key_state = nullptr;
    SDL_Window *instance = nullptr;
    SDL_GPUDevice *gpu = nullptr;
    SDL_GPUCommandBuffer *command_buffer = nullptr;
    SDL_GPURenderPass *render_pass = nullptr;

  protected:
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
