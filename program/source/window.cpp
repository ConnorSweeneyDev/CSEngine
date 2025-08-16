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
  window::frame::frame(const std::string &title_, const unsigned int starting_width_,
                       const unsigned int starting_height_, const bool fullscreen_)
    : title(title_), starting_width(starting_width_), starting_height(starting_height_), width(starting_width_),
      height(starting_height_), fullscreen(fullscreen_)
  {
  }

  void window::frame::handle_move(SDL_Window *instance)
  {
    if (fullscreen) return;
    if (!SDL_GetWindowPosition(instance, &left, &top))
      throw utility::sdl_exception("Could not get window position for window {}", title);
    display_index = SDL_GetDisplayForWindow(instance);
    if (display_index == 0) throw utility::sdl_exception("Could not get display index for window {}", title);
  }

  void window::frame::disable_fullscreen(SDL_Window *instance)
  {
    if (!SDL_SetWindowBordered(instance, true))
      throw utility::sdl_exception("Could not set bordered for window {}", title);
    if (!SDL_SetWindowSize(instance, static_cast<int>(starting_width), static_cast<int>(starting_height)))
      throw utility::sdl_exception("Could not set window size to ({}, {})", starting_width, starting_height);
    width = starting_width;
    height = starting_height;
    if (!SDL_SetWindowPosition(instance, left, top))
      throw utility::sdl_exception("Could not set window position to ({}, {})", left, top);
  }

  void window::frame::enable_fullscreen(SDL_Window *instance)
  {
    SDL_Rect display_bounds;
    if (!SDL_GetDisplayBounds(display_index, &display_bounds))
      throw utility::sdl_exception("Could not get display bounds for display {}", display_index);
    if (!SDL_SetWindowBordered(instance, false))
      throw utility::sdl_exception("Could not set borderless for window {}", title);
    if (!SDL_SetWindowSize(instance, display_bounds.w, display_bounds.h))
      throw utility::sdl_exception("Could not set window size to ({}, {}) on display {}", display_bounds.w,
                                   display_bounds.h, display_index);
    width = static_cast<unsigned int>(display_bounds.w);
    height = static_cast<unsigned int>(display_bounds.h);
    if (!SDL_SetWindowPosition(instance, SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
                               SDL_WINDOWPOS_CENTERED_DISPLAY(display_index)))
      throw utility::sdl_exception("Could not set window position centered on display {}", display_index);
  }

  window::graphics::graphics(const bool vsync_) : vsync(vsync_) {}

  void window::graphics::disable_vsync(const std::string &title)
  {
    if (SDL_WindowSupportsGPUPresentMode(gpu, instance, SDL_GPU_PRESENTMODE_IMMEDIATE))
      if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                         SDL_GPU_PRESENTMODE_IMMEDIATE))
        throw utility::sdl_exception("Could not disable VSYNC for window {}", title);
  }

  void window::graphics::enable_vsync(const std::string &title)
  {
    if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
      throw utility::sdl_exception("Could not enable VSYNC for window {}", title);
  }

  window::window(const std::string &title_, const unsigned int starting_width_, const unsigned int starting_height_,
                 const bool fullscreen_, const bool vsync_)
    : frame(title_, starting_width_, starting_height_, fullscreen_), graphics(vsync_)
  {
    if (title_.empty()) throw cse::utility::exception("Window title cannot be empty");
  }

  window::~window()
  {
    key_state = nullptr;
    handle_input = nullptr;
  }

  void window::toggle_fullscreen()
  {
    if (frame.fullscreen)
      frame.disable_fullscreen(graphics.instance);
    else
      frame.enable_fullscreen(graphics.instance);
    frame.fullscreen = !frame.fullscreen;
  }

  void window::toggle_vsync()
  {
    if (graphics.vsync)
      graphics.disable_vsync(frame.title);
    else
      graphics.enable_vsync(frame.title);
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

    graphics.instance = SDL_CreateWindow(frame.title.c_str(), static_cast<int>(frame.starting_width),
                                         static_cast<int>(frame.starting_height), SDL_WINDOW_HIDDEN);
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
        case SDL_EVENT_QUIT: running = false; break;
        case SDL_EVENT_WINDOW_MOVED: frame.handle_move(graphics.instance); break;
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

    SDL_GPUColorTargetInfo color_target_info(swapchain_texture, Uint32(), Uint32(), {0.1f, 0.1f, 0.1f, 1.0f},
                                             SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE);
    graphics.render_pass = SDL_BeginGPURenderPass(graphics.command_buffer, &color_target_info, 1, nullptr);
    if (!graphics.render_pass) throw cse::utility::sdl_exception("Could not begin GPU render pass");
    SDL_GPUViewport viewport(0.0f, 0.0f, static_cast<float>(frame.width), static_cast<float>(frame.height));
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
