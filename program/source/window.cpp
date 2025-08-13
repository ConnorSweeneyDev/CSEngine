#include "window.hpp"

#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_video.h"

#include "exception.hpp"

namespace cse::core
{
  window::frame::frame(const std::string &title_, const int starting_width_, const int starting_height_,
                       const bool fullscreen_)
    : title(title_), starting_width(starting_width_), starting_height(starting_height_), width(starting_width_),
      height(starting_height_), fullscreen(fullscreen_)
  {
  }

  window::graphics::graphics(const bool vsync_) : vsync(vsync_) {}

  window::window(const std::string &title_, const int starting_width_, const int starting_height_,
                 const bool fullscreen_, const bool vsync_)
    : frame(title_, starting_width_, starting_height_, fullscreen_), graphics(vsync_)
  {
    if (title_.empty()) throw cse::utility::exception("Window title cannot be empty");
    if (starting_width_ <= 0 || starting_height_ <= 0)
      throw cse::utility::exception("Window dimensions must be greater than zero");
  }

  window::~window()
  {
    key_state = nullptr;
    handle_input = nullptr;
  }

  void window::quit() { running = false; }

  void window::move()
  {
    if (frame.fullscreen) return;
    if (!SDL_GetWindowPosition(graphics.instance, &frame.left, &frame.top))
      throw utility::sdl_exception("Could not get window position for window {}", frame.title);
    frame.display_index = SDL_GetDisplayForWindow(graphics.instance);
    if (frame.display_index == 0)
      throw utility::sdl_exception("Could not get display index for window {}", frame.title);
  }

  void window::toggle_fullscreen()
  {
    if (frame.fullscreen)
    {
      if (!SDL_SetWindowBordered(graphics.instance, true))
        throw utility::sdl_exception("Could not set bordered for window {}", frame.title);
      if (!SDL_SetWindowSize(graphics.instance, frame.starting_width, frame.starting_height))
        throw utility::sdl_exception("Could not set window size to ({}, {})", frame.starting_width,
                                     frame.starting_height);
      frame.width = frame.starting_width;
      frame.height = frame.starting_height;
      if (!SDL_SetWindowPosition(graphics.instance, frame.left, frame.top))
        throw utility::sdl_exception("Could not set window position to ({}, {})", frame.left, frame.top);
    }
    else
    {
      SDL_Rect display_bounds;
      if (!SDL_GetDisplayBounds(frame.display_index, &display_bounds))
        throw utility::sdl_exception("Could not get display bounds for display {}", frame.display_index);
      if (!SDL_SetWindowBordered(graphics.instance, false))
        throw utility::sdl_exception("Could not set borderless for window {}", frame.title);
      if (!SDL_SetWindowSize(graphics.instance, display_bounds.w, display_bounds.h))
        throw utility::sdl_exception("Could not set window size to ({}, {}) on display {}", display_bounds.w,
                                     display_bounds.h, frame.display_index);
      frame.width = display_bounds.w;
      frame.height = display_bounds.h;
      if (!SDL_SetWindowPosition(graphics.instance, SDL_WINDOWPOS_CENTERED_DISPLAY(frame.display_index),
                                 SDL_WINDOWPOS_CENTERED_DISPLAY(frame.display_index)))
        throw utility::sdl_exception("Could not set window position centered on display {}", frame.display_index);
    }
    frame.fullscreen = !frame.fullscreen;
  }

  void window::toggle_vsync()
  {
    if (graphics.vsync)
    {
      if (SDL_WindowSupportsGPUPresentMode(graphics.gpu, graphics.instance, SDL_GPU_PRESENTMODE_IMMEDIATE))
        if (!SDL_SetGPUSwapchainParameters(graphics.gpu, graphics.instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                           SDL_GPU_PRESENTMODE_IMMEDIATE))
          throw utility::sdl_exception("Could not disable VSYNC for window {}", frame.title);
    }
    else
    {
      if (!SDL_SetGPUSwapchainParameters(graphics.gpu, graphics.instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                         SDL_GPU_PRESENTMODE_VSYNC))
        throw utility::sdl_exception("Could not enable VSYNC for window {}", frame.title);
    }
    graphics.vsync = !graphics.vsync;
  }

  void window::initialize()
  {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game"))
      throw cse::utility::sdl_exception("Could not set app metadata type for window {}", frame.title);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING, "Connor.Sweeney.Engine"))
      throw cse::utility::sdl_exception("Could not set app metadata identifier for window {}", frame.title);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "CSEngine"))
      throw cse::utility::sdl_exception("Could not set app metadata name for window {}", frame.title);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, "0.0.0"))
      throw cse::utility::sdl_exception("Could not set app metadata version for window {}", frame.title);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "Connor Sweeney"))
      throw cse::utility::sdl_exception("Could not set app metadata creator for window {}", frame.title);
    if (!SDL_Init(SDL_INIT_VIDEO))
      throw cse::utility::sdl_exception("SDL could not be initialized for window {}", frame.title);

    graphics.instance =
      SDL_CreateWindow(frame.title.c_str(), frame.starting_width, frame.starting_height, SDL_WINDOW_HIDDEN);
    if (!graphics.instance) throw cse::utility::sdl_exception("Could not create window {}", frame.title);
    frame.display_index = SDL_GetPrimaryDisplay();
    if (frame.display_index == 0)
      throw cse::utility::sdl_exception("Could not get primary display for window {}", frame.title);
    frame.left = SDL_WINDOWPOS_CENTERED_DISPLAY(frame.display_index);
    frame.top = SDL_WINDOWPOS_CENTERED_DISPLAY(frame.display_index);
    if (!SDL_SetWindowPosition(graphics.instance, frame.left, frame.top))
      throw cse::utility::sdl_exception("Could not set window {} position to ({}, {})", frame.title, frame.left,
                                        frame.top);
    if (frame.fullscreen)
    {
      frame.fullscreen = !frame.fullscreen;
      toggle_fullscreen();
    }

    graphics.gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, false, nullptr);
    if (!graphics.gpu) throw cse::utility::sdl_exception("Could not create GPU device for window {}", frame.title);
    if (!SDL_ClaimWindowForGPUDevice(graphics.gpu, graphics.instance))
      throw cse::utility::sdl_exception("Could not claim window for GPU device for window {}", frame.title);
    if (!SDL_SetGPUSwapchainParameters(graphics.gpu, graphics.instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                       SDL_GPU_PRESENTMODE_VSYNC))
      throw cse::utility::sdl_exception("Could not enable VSYNC for window {}", frame.title);
    if (!graphics.vsync)
    {
      graphics.vsync = !graphics.vsync;
      toggle_vsync();
    }

    SDL_ShowWindow(graphics.instance);
    running = true;
  }

  void window::cleanup()
  {
    SDL_ReleaseWindowFromGPUDevice(graphics.gpu, graphics.instance);
    SDL_DestroyGPUDevice(graphics.gpu);
    SDL_DestroyWindow(graphics.instance);

    graphics.render_pass = nullptr;
    graphics.command_buffer = nullptr;
    graphics.gpu = nullptr;
    graphics.instance = nullptr;

    SDL_Quit();
  }

  void window::input()
  {
    key_state = SDL_GetKeyboardState(nullptr);
    SDL_Event event = {};
    while (SDL_PollEvent(&event)) switch (event.type)
      {
        case SDL_EVENT_QUIT: quit(); break;
        case SDL_EVENT_WINDOW_MOVED: move(); break;
        case SDL_EVENT_KEY_DOWN:
          if (handle_input) handle_input(event.key);
          break;
      }
  }

  bool window::start_render()
  {
    graphics.command_buffer = SDL_AcquireGPUCommandBuffer(graphics.gpu);
    if (!graphics.command_buffer) throw cse::utility::sdl_exception("Could not acquire GPU command buffer");

    SDL_GPUTexture *swapchain_texture = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(graphics.command_buffer, graphics.instance, &swapchain_texture, nullptr,
                                               nullptr))
      throw cse::utility::sdl_exception("Could not acquire GPU swapchain texture");
    if (!swapchain_texture)
    {
      if (!SDL_SubmitGPUCommandBuffer(graphics.command_buffer))
        throw cse::utility::sdl_exception("Could not submit GPU command buffer");
      return false;
    }

    SDL_GPUColorTargetInfo color_target_info = {};
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;
    color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target_info.texture = swapchain_texture;
    color_target_info.clear_color = {0.1f, 0.1f, 0.1f, 1.0f};
    graphics.render_pass = SDL_BeginGPURenderPass(graphics.command_buffer, &color_target_info, 1, nullptr);
    if (!graphics.render_pass) throw cse::utility::sdl_exception("Could not begin GPU render pass");

    SDL_GPUViewport viewport = {};
    viewport.w = static_cast<float>(frame.width);
    viewport.h = static_cast<float>(frame.height);
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    SDL_SetGPUViewport(graphics.render_pass, &viewport);

    return true;
  }

  void window::end_render()
  {
    SDL_EndGPURenderPass(graphics.render_pass);
    if (!SDL_SubmitGPUCommandBuffer(graphics.command_buffer))
      throw cse::utility::sdl_exception("Could not submit GPU command buffer");
  }
}
