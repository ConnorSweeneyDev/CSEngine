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
  window::graphics::graphics(const std::string &title_, const int starting_width_, const int starting_height_)
    : title(title_), width(starting_width_), height(starting_height_), starting_width(starting_width_),
      starting_height(starting_height_)
  {
  }

  void window::graphics::initialize_window()
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

    instance = SDL_CreateWindow(title.c_str(), starting_width, starting_height, SDL_WINDOW_HIDDEN);
    if (!instance) throw cse::utility::sdl_exception("Could not create window {}", title);
    display_index = SDL_GetPrimaryDisplay();
    if (display_index == 0) throw cse::utility::sdl_exception("Could not get primary display for window {}", title);
    left = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    top = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    if (!SDL_SetWindowPosition(instance, left, top))
      throw cse::utility::sdl_exception("Could not set window {} position to ({}, {})", title, left, top);
  }

  void window::graphics::initialize_gpu()
  {
    gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, false, nullptr);
    if (!gpu) throw cse::utility::sdl_exception("Could not create GPU device for window {}", title);
    if (!SDL_ClaimWindowForGPUDevice(gpu, instance))
      throw cse::utility::sdl_exception("Could not claim window for GPU device for window {}", title);
    if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
      throw cse::utility::sdl_exception("Could not enable VSYNC for window {}", title);
  }

  void window::graphics::show_window() const { SDL_ShowWindow(instance); }

  void window::graphics::generate_command_buffer()
  {
    command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    if (!command_buffer) throw cse::utility::sdl_exception("Could not acquire GPU command buffer");
  }

  bool window::graphics::generate_render_pass()
  {
    SDL_GPUTexture *swapchain_texture = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, instance, &swapchain_texture, nullptr, nullptr))
      throw cse::utility::sdl_exception("Could not acquire GPU swapchain texture");
    if (!swapchain_texture)
    {
      if (!SDL_SubmitGPUCommandBuffer(command_buffer))
        throw cse::utility::sdl_exception("Could not submit GPU command buffer");
      return false;
    }

    SDL_GPUColorTargetInfo color_target_info = {};
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;
    color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target_info.texture = swapchain_texture;
    color_target_info.clear_color = {0.1f, 0.1f, 0.1f, 1.0f};
    render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr);
    if (!render_pass) throw cse::utility::sdl_exception("Could not begin GPU render pass");

    return true;
  }

  void window::graphics::generate_viewport()
  {
    SDL_GPUViewport viewport = {};
    viewport.w = static_cast<float>(width);
    viewport.h = static_cast<float>(height);
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    SDL_SetGPUViewport(render_pass, &viewport);
  }

  void window::graphics::end_render_and_submit()
  {
    SDL_EndGPURenderPass(render_pass);
    if (!SDL_SubmitGPUCommandBuffer(command_buffer))
      throw cse::utility::sdl_exception("Could not submit GPU command buffer");
  }

  void window::graphics::detect_display_index()
  {
    if (!SDL_GetWindowPosition(instance, &left, &top))
      throw utility::sdl_exception("Could not get window position for window at ({}, {})", left, top);
    display_index = SDL_GetDisplayForWindow(instance);
    if (display_index == 0) throw utility::sdl_exception("Could not get display index");
  }

  void window::graphics::change_to_windowed()
  {
    if (!SDL_SetWindowBordered(instance, true))
      throw utility::sdl_exception("Could not set bordered for window {}", title);
    if (!SDL_SetWindowSize(instance, starting_width, starting_height))
      throw utility::sdl_exception("Could not set window size to ({}, {})", starting_width, starting_height);
    width = starting_width;
    height = starting_height;
    if (!SDL_SetWindowPosition(instance, left, top))
      throw utility::sdl_exception("Could not set window position to ({}, {})", left, top);
  }

  void window::graphics::change_to_fullscreen()
  {
    SDL_Rect display_bounds;
    if (!SDL_GetDisplayBounds(display_index, &display_bounds))
      throw utility::sdl_exception("Could not get display bounds for display {}", display_index);
    if (!SDL_SetWindowBordered(instance, false))
      throw utility::sdl_exception("Could not set borderless for window {}", title);
    if (!SDL_SetWindowSize(instance, display_bounds.w, display_bounds.h))
      throw utility::sdl_exception("Could not set window size to ({}, {}) on display {}", display_bounds.w,
                                   display_bounds.h, display_index);
    width = display_bounds.w;
    height = display_bounds.h;
    if (!SDL_SetWindowPosition(instance, SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
                               SDL_WINDOWPOS_CENTERED_DISPLAY(display_index)))
      throw utility::sdl_exception("Could not set window position centered on display {}", display_index);
  }

  void window::graphics::change_to_immediate()
  {
    if (SDL_WindowSupportsGPUPresentMode(gpu, instance, SDL_GPU_PRESENTMODE_IMMEDIATE))
      if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                         SDL_GPU_PRESENTMODE_IMMEDIATE))
        throw utility::sdl_exception("Could not disable VSYNC for window {}", title);
  }

  void window::graphics::change_to_vsync()
  {
    if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
      throw utility::sdl_exception("Could not enable VSYNC for window {}", title);
  }

  void window::graphics::cleanup_window()
  {
    SDL_ReleaseWindowFromGPUDevice(gpu, instance);
    SDL_DestroyGPUDevice(gpu);
    SDL_DestroyWindow(instance);

    render_pass = nullptr;
    command_buffer = nullptr;
    gpu = nullptr;
    instance = nullptr;
  }

  window::window(const std::string &title_, const int starting_width_, const int starting_height_,
                 const bool fullscreen_, const bool vsync_)
    : fullscreen(fullscreen_), vsync(vsync_), graphics(title_, starting_width_, starting_height_)
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

  bool window::is_running() const { return running; }

  int window::get_width() const { return graphics.width; }

  int window::get_height() const { return graphics.height; }

  const bool *window::get_key_state() const
  {
    if (!key_state) throw cse::utility::sdl_exception("Key state is not set for window");
    return key_state;
  }

  SDL_Window *window::get_instance() const
  {
    if (!graphics.instance) throw cse::utility::sdl_exception("Window instance is not set");
    return graphics.instance;
  }

  SDL_GPUDevice *window::get_gpu() const
  {
    if (!graphics.gpu) throw cse::utility::sdl_exception("GPU device is not set for window");
    return graphics.gpu;
  }

  SDL_GPUCommandBuffer *window::get_command_buffer() const
  {
    if (!graphics.command_buffer) throw cse::utility::sdl_exception("Command buffer is not set for window");
    return graphics.command_buffer;
  }

  SDL_GPURenderPass *window::get_render_pass() const
  {
    if (!graphics.render_pass) throw cse::utility::sdl_exception("Render pass is not set for window");
    return graphics.render_pass;
  }

  void window::initialize()
  {
    graphics.initialize_window();
    if (fullscreen)
    {
      fullscreen = !fullscreen;
      toggle_fullscreen();
    }
    graphics.initialize_gpu();
    if (!vsync)
    {
      vsync = !vsync;
      toggle_vsync();
    }
    graphics.show_window();
    running = true;
  }

  void window::cleanup()
  {
    graphics.cleanup_window();
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
    graphics.generate_command_buffer();
    if (!graphics.generate_render_pass()) return false;
    graphics.generate_viewport();
    return true;
  }

  void window::end_render() { graphics.end_render_and_submit(); }

  void window::quit() { running = false; }

  void window::move()
  {
    if (fullscreen) return;
    graphics.detect_display_index();
  }

  void window::toggle_fullscreen()
  {
    if (fullscreen)
      graphics.change_to_windowed();
    else
      graphics.change_to_fullscreen();
    fullscreen = !fullscreen;
  }

  void window::toggle_vsync()
  {
    if (vsync)
      graphics.change_to_immediate();
    else
      graphics.change_to_vsync();
    vsync = !vsync;
  }
}
