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

namespace cse::base
{
  window::graphics::graphics(int starting_width_, int starting_height_)
    : width(starting_width_), height(starting_height_)
  {
  }

  window::window(const std::string &title_, int starting_width_, int starting_height_, bool fullscreen_, bool vsync_)
    : title(title_), starting_width(starting_width_), starting_height(starting_height_), fullscreen(fullscreen_),
      vsync(vsync_), graphics(starting_width_, starting_height_)
  {
    if (title.empty()) throw cse::utility::exception("Window title cannot be empty");
    if (starting_width <= 0 || starting_height <= 0)
      throw cse::utility::exception("Window dimensions must be greater than zero");
  }

  window::~window()
  {
    graphics.render_pass = nullptr;
    graphics.command_buffer = nullptr;
    graphics.gpu = nullptr;
    graphics.instance = nullptr;
    key_state = nullptr;
    handle_input = nullptr;
  }

  bool window::is_running() const { return running; }

  const bool *window::get_key_state() const
  {
    if (!key_state) throw cse::utility::sdl_exception("Key state is not set for window {}", title);
    return key_state;
  }

  auto window::get_graphics() -> struct graphics const { return graphics; }

  void window::initialize()
  {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game"))
      throw cse::utility::sdl_exception("Could not set app metadata type for window {}", title);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING, "Connor.Sweeney.Engine"))
      throw cse::utility::sdl_exception("Could not set app metadata identifier for window {}", title);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "CSEngine"))
      throw cse::utility::sdl_exception("Could not set app metadata name for window {}", title);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, "0.0.0"))
      throw cse::utility::sdl_exception("Could not set app metadata version for window {}", title);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "Connor Sweeney"))
      throw cse::utility::sdl_exception("Could not set app metadata creator for window {}", title);
    if (!SDL_Init(SDL_INIT_VIDEO))
      throw cse::utility::sdl_exception("SDL could not be initialized for window {}", title);

    graphics.instance = SDL_CreateWindow(title.c_str(), starting_width, starting_height, SDL_WINDOW_HIDDEN);
    if (!graphics.instance) throw cse::utility::sdl_exception("Could not create window {}", title);
    graphics.display_index = SDL_GetPrimaryDisplay();
    if (graphics.display_index == 0)
      throw cse::utility::sdl_exception("Could not get primary display for window {}", title);
    graphics.left = SDL_WINDOWPOS_CENTERED_DISPLAY(graphics.display_index);
    graphics.top = SDL_WINDOWPOS_CENTERED_DISPLAY(graphics.display_index);
    if (!SDL_SetWindowPosition(graphics.instance, graphics.left, graphics.top))
      throw cse::utility::sdl_exception("Could not set window {} position to ({}, {})", title, graphics.left,
                                        graphics.top);
    if (fullscreen)
    {
      fullscreen = !fullscreen;
      toggle_fullscreen();
    }

    graphics.gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, false, nullptr);
    if (!graphics.gpu) throw cse::utility::sdl_exception("Could not create GPU device for window {}", title);
    if (!SDL_ClaimWindowForGPUDevice(graphics.gpu, graphics.instance))
      throw cse::utility::sdl_exception("Could not claim window for GPU device for window {}", title);
    if (!SDL_SetGPUSwapchainParameters(graphics.gpu, graphics.instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                       SDL_GPU_PRESENTMODE_VSYNC))
      throw cse::utility::sdl_exception("Could not enable VSYNC for window {}", title);
    if (!vsync)
    {
      vsync = !vsync;
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
    SDL_Quit();
  }

  void window::input()
  {
    key_state = SDL_GetKeyboardState(nullptr);
    SDL_Event event = {};
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
        case SDL_EVENT_QUIT: quit(); break;
        case SDL_EVENT_WINDOW_MOVED: move(); break;
        case SDL_EVENT_KEY_DOWN:
          if (handle_input) handle_input(event.key);
          break;
      }
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
    viewport.w = static_cast<float>(graphics.width);
    viewport.h = static_cast<float>(graphics.height);
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

  void window::quit() { running = false; }

  void window::move()
  {
    if (fullscreen) return;

    if (!SDL_GetWindowPosition(graphics.instance, &graphics.left, &graphics.top))
      throw utility::sdl_exception("Could not get window position for window at ({}, {})", graphics.left, graphics.top);
    graphics.display_index = SDL_GetDisplayForWindow(graphics.instance);
    if (graphics.display_index == 0) throw utility::sdl_exception("Could not get display index");
  }

  void window::toggle_fullscreen()
  {
    if (fullscreen)
    {
      if (!SDL_SetWindowBordered(graphics.instance, true))
        throw utility::sdl_exception("Could not set window bordered");
      if (!SDL_SetWindowSize(graphics.instance, starting_width, starting_height))
        throw utility::sdl_exception("Could not set window size to ({}, {})", starting_width, starting_height);
      graphics.width = starting_width;
      graphics.height = starting_height;
      if (!SDL_SetWindowPosition(graphics.instance, graphics.left, graphics.top))
        throw utility::sdl_exception("Could not set window position to ({}, {})", graphics.left, graphics.top);
    }
    else
    {
      SDL_Rect display_bounds;
      if (!SDL_GetDisplayBounds(graphics.display_index, &display_bounds))
        throw utility::sdl_exception("Could not get display bounds for display {}", graphics.display_index);
      if (!SDL_SetWindowBordered(graphics.instance, false))
        throw utility::sdl_exception("Could not set window borderless");
      if (!SDL_SetWindowSize(graphics.instance, display_bounds.w, display_bounds.h))
        throw utility::sdl_exception("Could not set window size to ({}, {}) on display {}", display_bounds.w,
                                     display_bounds.h, graphics.display_index);
      graphics.width = display_bounds.w;
      graphics.height = display_bounds.h;
      if (!SDL_SetWindowPosition(graphics.instance, SDL_WINDOWPOS_CENTERED_DISPLAY(graphics.display_index),
                                 SDL_WINDOWPOS_CENTERED_DISPLAY(graphics.display_index)))
        throw utility::sdl_exception("Could not set window position centered on display {}", graphics.display_index);
    }
    fullscreen = !fullscreen;
  }

  void window::toggle_vsync()
  {
    if (vsync)
    {
      if (SDL_WindowSupportsGPUPresentMode(graphics.gpu, graphics.instance, SDL_GPU_PRESENTMODE_IMMEDIATE))
        if (!SDL_SetGPUSwapchainParameters(graphics.gpu, graphics.instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                           SDL_GPU_PRESENTMODE_IMMEDIATE))
          throw utility::sdl_exception("Could not disable VSYNC for window {}", title);
    }
    else if (!SDL_SetGPUSwapchainParameters(graphics.gpu, graphics.instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                            SDL_GPU_PRESENTMODE_VSYNC))
      throw utility::sdl_exception("Could not enable VSYNC for window {}", title);
    vsync = !vsync;
  }
}
