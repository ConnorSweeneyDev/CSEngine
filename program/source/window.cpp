#include "window.hpp"

#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_video.h"

#include "exception.hpp"

namespace cse::base
{
  window::window(const std::string &i_title, int i_starting_width, int i_starting_height, bool i_fullscreen,
                 bool i_vsync)
    : width(i_starting_width), height(i_starting_height), title(i_title), starting_width(i_starting_width),
      starting_height(i_starting_height)
  {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata("CSEngine", "0.0.0", "Connor.Sweeney.Engine");
    if (!SDL_Init(SDL_INIT_VIDEO)) throw utility::sdl_exception("SDL could not be initialized for window {}", i_title);

    instance = SDL_CreateWindow(i_title.c_str(), i_starting_width, i_starting_height, SDL_WINDOW_HIDDEN);
    if (!instance) throw utility::sdl_exception("Could not create window {}", i_title);

    display_index = SDL_GetPrimaryDisplay();
    if (display_index == 0) throw utility::sdl_exception("Could not get primary display for window {}", i_title);
    left = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    top = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    if (!SDL_SetWindowPosition(instance, left, top))
      throw utility::sdl_exception("Could not set window {} position to ({}, {})", i_title, left, top);
    if (i_fullscreen) handle_fullscreen();

    gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, false, nullptr);
    if (!gpu) throw utility::sdl_exception("Could not create GPU device for window {}", i_title);
    if (!SDL_ClaimWindowForGPUDevice(gpu, instance))
      throw utility::sdl_exception("Could not claim window for GPU device for window {}", i_title);
    if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
      throw utility::sdl_exception("Could not enable VSYNC for window {}", title);
    if (!i_vsync) handle_vsync();

    SDL_ShowWindow(instance);
    running = true;
  }

  window::~window()
  {
    SDL_ReleaseWindowFromGPUDevice(gpu, instance);
    SDL_DestroyGPUDevice(gpu);
    SDL_DestroyWindow(instance);
    SDL_Quit();
    running = false;
  }

  bool window::is_running() { return running; }

  void window::input()
  {
    key_state = SDL_GetKeyboardState(nullptr);
    SDL_Event event = {};
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
        case SDL_EVENT_QUIT: handle_quit(); break;
        case SDL_EVENT_WINDOW_MOVED: handle_move(); break;
        case SDL_EVENT_KEY_DOWN:
          switch (event.key.scancode)
          {
            case SDL_SCANCODE_ESCAPE: handle_quit(); break;
            case SDL_SCANCODE_F11: handle_fullscreen(); break;
            case SDL_SCANCODE_F12: handle_vsync(); break;
            default: break;
          }
        default: break;
      }
    }
  }

  bool window::start_render()
  {
    command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    if (!command_buffer) throw utility::sdl_exception("Could not acquire GPU command buffer");

    SDL_GPUTexture *swapchain_texture = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, instance, &swapchain_texture, nullptr, nullptr))
      throw utility::sdl_exception("Could not acquire GPU swapchain texture");
    if (!swapchain_texture)
    {
      if (!SDL_SubmitGPUCommandBuffer(command_buffer))
        throw utility::sdl_exception("Could not submit GPU command buffer");
      return false;
    }

    SDL_GPUColorTargetInfo color_target_info = {};
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;
    color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target_info.texture = swapchain_texture;
    color_target_info.clear_color = {0.1f, 0.1f, 0.1f, 1.0f};
    render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr);
    if (!render_pass) throw utility::sdl_exception("Could not begin GPU render pass");

    SDL_GPUViewport viewport = {};
    viewport.w = static_cast<float>(width);
    viewport.h = static_cast<float>(height);
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    SDL_SetGPUViewport(render_pass, &viewport);

    return true;
  }

  void window::end_render()
  {
    SDL_EndGPURenderPass(render_pass);
    if (!SDL_SubmitGPUCommandBuffer(command_buffer))
      throw utility::sdl_exception("Could not submit GPU command buffer");
  }

  void window::handle_quit() { running = false; }

  void window::handle_move()
  {
    if (fullscreen) return;

    if (!SDL_GetWindowPosition(instance, &left, &top))
      throw utility::sdl_exception("Could not get window position for window at ({}, {})", left, top);
    display_index = SDL_GetDisplayForWindow(instance);
    if (display_index == 0) throw utility::sdl_exception("Could not get display index");
  }

  void window::handle_fullscreen()
  {
    if (fullscreen)
    {
      if (!SDL_SetWindowBordered(instance, true)) throw utility::sdl_exception("Could not set window bordered");
      if (!SDL_SetWindowSize(instance, starting_width, starting_height))
        throw utility::sdl_exception("Could not set window size to ({}, {})", starting_width, starting_height);
      width = starting_width;
      height = starting_height;
      if (!SDL_SetWindowPosition(instance, left, top))
        throw utility::sdl_exception("Could not set window position to ({}, {})", left, top);
    }
    else
    {
      SDL_Rect display_bounds;
      if (!SDL_GetDisplayBounds(display_index, &display_bounds))
        throw utility::sdl_exception("Could not get display bounds for display {}", display_index);
      if (!SDL_SetWindowBordered(instance, false)) throw utility::sdl_exception("Could not set window borderless");
      if (!SDL_SetWindowSize(instance, display_bounds.w, display_bounds.h))
        throw utility::sdl_exception("Could not set window size to ({}, {}) on display {}", display_bounds.w,
                                     display_bounds.h, display_index);
      width = display_bounds.w;
      height = display_bounds.h;
      if (!SDL_SetWindowPosition(instance, SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
                                 SDL_WINDOWPOS_CENTERED_DISPLAY(display_index)))
        throw utility::sdl_exception("Could not set window position centered on display {}", display_index);
    }
    fullscreen = !fullscreen;
  }

  void window::handle_vsync()
  {
    if (vsync)
    {
      if (SDL_WindowSupportsGPUPresentMode(gpu, instance, SDL_GPU_PRESENTMODE_IMMEDIATE))
        if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                           SDL_GPU_PRESENTMODE_IMMEDIATE))
          throw utility::sdl_exception("Could not disable VSYNC for window {}", title);
    }
    else if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
      throw utility::sdl_exception("Could not enable VSYNC for window {}", title);
    vsync = !vsync;
  }
}
