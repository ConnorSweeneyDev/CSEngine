#include "window.hpp"

#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"

#include "exception.hpp"

namespace cse::core
{
  window::graphics::graphics(const std::string &title_, const unsigned int starting_width_,
                             const unsigned int starting_height_, const bool fullscreen_, const bool vsync_)
    : fullscreen(fullscreen_), vsync(vsync_), title(title_), starting_width(starting_width_),
      starting_height(starting_height_), width(starting_width_), height(starting_height_)
  {
    fullscreen.on_change = [this]()
    {
      if (fullscreen)
      {
        SDL_Rect display_bounds;
        if (!SDL_GetDisplayBounds(display_index, &display_bounds))
          throw utility::sdl_exception("Could not get bounds for display ({}) for window '{}'", display_index, title);
        if (!SDL_SetWindowBordered(instance, false))
          throw utility::sdl_exception("Could not set borderless for window '{}'", title);
        if (!SDL_SetWindowSize(instance, display_bounds.w, display_bounds.h))
          throw utility::sdl_exception("Could not set size to ({}, {}) on display ({}) for window '{}'",
                                       display_bounds.w, display_bounds.h, display_index, title);
        width = static_cast<unsigned int>(display_bounds.w);
        height = static_cast<unsigned int>(display_bounds.h);
        if (!SDL_SetWindowPosition(instance, SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
                                   SDL_WINDOWPOS_CENTERED_DISPLAY(display_index)))
          throw utility::sdl_exception("Could not set position centered on display ({}) for window '{}'",
                                       display_index);
      }
      else
      {
        if (!SDL_SetWindowBordered(instance, true))
          throw utility::sdl_exception("Could not set bordered for window '{}'", title);
        if (!SDL_SetWindowSize(instance, static_cast<int>(starting_width), static_cast<int>(starting_height)))
          throw utility::sdl_exception("Could not set size to ({}, {}) for window '{}'", starting_width,
                                       starting_height, title);
        width = starting_width;
        height = starting_height;
        if (!SDL_SetWindowPosition(instance, left, top))
          throw utility::sdl_exception("Could not set position to ({}, {}) for window '{}'", left, top, title);
      }
    };

    vsync.on_change = [this]()
    {
      if (vsync)
      {
        if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
          throw utility::sdl_exception("Could not enable VSYNC for window '{}'", title);
      }
      else if (SDL_WindowSupportsGPUPresentMode(gpu, instance, SDL_GPU_PRESENTMODE_IMMEDIATE))
        if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                           SDL_GPU_PRESENTMODE_IMMEDIATE))
          throw utility::sdl_exception("Could not disable VSYNC for window '{}'", title);
    };
  }

  window::graphics::graphics::~graphics()
  {
    fullscreen.on_change = nullptr;
    vsync.on_change = nullptr;
  }

  void window::graphics::handle_move()
  {
    if (fullscreen) return;
    if (!SDL_GetWindowPosition(instance, &left, &top))
      throw utility::sdl_exception("Could not get position for window '{}'", title);
    display_index = SDL_GetDisplayForWindow(instance);
    if (display_index == 0) throw utility::sdl_exception("Could not get display index for window '{}'", title);
  }

  window::window(const std::string &title_, const unsigned int starting_width_, const unsigned int starting_height_,
                 const bool fullscreen_, const bool vsync_)
    : graphics(title_, starting_width_, starting_height_, fullscreen_, vsync_)
  {
    if (title_.empty())
      throw cse::utility::exception("Window title cannot be empty for window with dimensions ({}, {})", starting_width_,
                                    starting_height_);
  }

  window::~window()
  {
    key_state = nullptr;
    handle_input = nullptr;
  }

  void window::initialize()
  {
    if (!graphics.fullscreen.on_change)
      throw cse::utility::exception("Fullscreen on_change callback must be set for window '{}'", graphics.title);
    if (!graphics.vsync.on_change)
      throw cse::utility::exception("Vsync on_change callback must be set for window '{}'", graphics.title);

    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game"))
      throw cse::utility::sdl_exception("Could not set app metadata type for window '{}'", graphics.title);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING, "Connor.Sweeney.Engine"))
      throw cse::utility::sdl_exception("Could not set app metadata identifier for window '{}'", graphics.title);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "CSEngine"))
      throw cse::utility::sdl_exception("Could not set app metadata name for window '{}'", graphics.title);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, "0.0.0"))
      throw cse::utility::sdl_exception("Could not set app metadata version for window '{}'", graphics.title);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "Connor Sweeney"))
      throw cse::utility::sdl_exception("Could not set app metadata creator for window '{}'", graphics.title);
    if (!SDL_Init(SDL_INIT_VIDEO))
      throw cse::utility::sdl_exception("SDL could not be initialized for window '{}'", graphics.title);

    graphics.instance = SDL_CreateWindow(graphics.title.c_str(), static_cast<int>(graphics.starting_width),
                                         static_cast<int>(graphics.starting_height), SDL_WINDOW_HIDDEN);
    if (!graphics.instance) throw cse::utility::sdl_exception("Could not create window '{}'", graphics.title);
    graphics.display_index = SDL_GetPrimaryDisplay();
    if (graphics.display_index == 0)
      throw cse::utility::sdl_exception("Could not get primary display for window '{}'", graphics.title);
    graphics.left = SDL_WINDOWPOS_CENTERED_DISPLAY(graphics.display_index);
    graphics.top = SDL_WINDOWPOS_CENTERED_DISPLAY(graphics.display_index);
    if (!SDL_SetWindowPosition(graphics.instance, graphics.left, graphics.top))
      throw cse::utility::sdl_exception("Could not set position to ({}, {}) for window '{}'", graphics.left,
                                        graphics.top, graphics.title);
    if (graphics.fullscreen) graphics.fullscreen.on_change();

    graphics.gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, false, nullptr);
    if (!graphics.gpu) throw cse::utility::sdl_exception("Could not create GPU device for window '{}'", graphics.title);
    if (!SDL_ClaimWindowForGPUDevice(graphics.gpu, graphics.instance))
      throw cse::utility::sdl_exception("Could not claim window for GPU device for window '{}'", graphics.title);
    if (!SDL_SetGPUSwapchainParameters(graphics.gpu, graphics.instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                       SDL_GPU_PRESENTMODE_VSYNC))
      throw cse::utility::sdl_exception("Could not enable VSYNC for window '{}'", graphics.title);
    if (!graphics.vsync) graphics.vsync.on_change();

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
        case SDL_EVENT_QUIT: running = false; break;
        case SDL_EVENT_WINDOW_MOVED: graphics.handle_move(); break;
        case SDL_EVENT_KEY_DOWN:
          if (handle_input) handle_input(event.key);
          break;
      }
  }

  bool window::start_render()
  {
    graphics.command_buffer = SDL_AcquireGPUCommandBuffer(graphics.gpu);
    if (!graphics.command_buffer)
      throw cse::utility::sdl_exception("Could not acquire GPU command buffer for window '{}'", graphics.title);

    SDL_GPUTexture *swapchain_texture = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(graphics.command_buffer, graphics.instance, &swapchain_texture, nullptr,
                                               nullptr))
      throw cse::utility::sdl_exception("Could not acquire GPU swapchain texture for window '{}'", graphics.title);
    if (!swapchain_texture)
    {
      if (!SDL_SubmitGPUCommandBuffer(graphics.command_buffer))
        throw cse::utility::sdl_exception("Could not submit GPU command buffer for window '{}'", graphics.title);
      return false;
    }

    SDL_GPUColorTargetInfo color_target_info(swapchain_texture, Uint32(), Uint32(), {0.1f, 0.1f, 0.1f, 1.0f},
                                             SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE);
    graphics.render_pass = SDL_BeginGPURenderPass(graphics.command_buffer, &color_target_info, 1, nullptr);
    if (!graphics.render_pass)
      throw cse::utility::sdl_exception("Could not begin GPU render pass for window '{}'", graphics.title);
    SDL_GPUViewport viewport(0.0f, 0.0f, static_cast<float>(graphics.width), static_cast<float>(graphics.height));
    SDL_SetGPUViewport(graphics.render_pass, &viewport);

    return true;
  }

  void window::end_render()
  {
    SDL_EndGPURenderPass(graphics.render_pass);
    if (!SDL_SubmitGPUCommandBuffer(graphics.command_buffer))
      throw cse::utility::sdl_exception("Could not submit GPU command buffer for window '{}'", graphics.title);
  }
}
