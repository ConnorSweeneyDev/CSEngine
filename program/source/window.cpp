#include "window.hpp"

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"

#include "utility.hpp"

namespace cse
{
  std::unique_ptr<Window> Window::create(const std::string &title, bool i_fullscreen, int i_width, int i_height)
  {
    bool expected = false;
    if (!initialized.compare_exchange_strong(expected, true))
    {
      utility::log("Window could not be created a second time", utility::FAILURE);
      return nullptr;
    }
    return std::unique_ptr<Window>(new Window(title, i_fullscreen, i_width, i_height));
  }

  bool Window::valid(const std::unique_ptr<Window> &window)
  {
    if (!window || !window->running) return false;
    return true;
  }

  Window::~Window()
  {
    if (!handle) return;
    SDL_DestroyWindow(handle);
    handle = nullptr;
    running = false;
    SDL_Quit();
    initialized.store(false);
  }

  int Window::input()
  {
    SDL_Event event = {};
    const bool *key_state = SDL_GetKeyboardState(nullptr);
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
        case SDL_EVENT_QUIT: return handle_quit();
        case SDL_EVENT_WINDOW_MOVED: return handle_move();
        case SDL_EVENT_KEY_DOWN:
          switch (event.key.scancode)
          {
            case SDL_SCANCODE_ESCAPE: return handle_quit();
            case SDL_SCANCODE_F11: return handle_fullscreen();
            default: return EXIT_SUCCESS;
          }
        default: return EXIT_SUCCESS;
      }
    }

    red_acceleration = 0.0f;
    if (key_state[SDL_SCANCODE_D]) red_acceleration -= 0.0005f;
    if (key_state[SDL_SCANCODE_F]) red_acceleration += 0.0005f;

    return EXIT_SUCCESS;
  }

  void Window::simulate()
  {
    previous_red = current_red;
    red_velocity += red_acceleration;
    current_red += red_velocity;
    if (current_red < 0.0f)
    {
      current_red = 0.0f;
      red_velocity = 0.0f;
    }
    if (current_red > 1.0f)
    {
      current_red = 1.0f;
      red_velocity = 0.0f;
    }
    interpolated_red = previous_red + ((current_red - previous_red) * static_cast<float>(simulation_alpha));
  }

  int Window::render()
  {
    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    if (!command_buffer) return utility::log("Could not acquire GPU command buffer", utility::SDL_FAILURE);
    SDL_GPUTexture *swapchain_texture = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, handle, &swapchain_texture, nullptr, nullptr))
      return utility::log("Could not acquire GPU swapchain texture", utility::SDL_FAILURE);
    SDL_GPUColorTargetInfo color_target = {};
    color_target.store_op = SDL_GPU_STOREOP_STORE;
    color_target.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target.texture = swapchain_texture;
    color_target.clear_color = {interpolated_red, 0.1f, 0.1f, 1.0f};
    std::vector<SDL_GPUColorTargetInfo> color_targets = {color_target};
    SDL_GPURenderPass *render_pass =
      SDL_BeginGPURenderPass(command_buffer, color_targets.data(), static_cast<Uint32>(color_targets.size()), nullptr);
    if (!render_pass) return utility::log("Could not begin GPU render pass", utility::SDL_FAILURE);
    SDL_EndGPURenderPass(render_pass);
    if (!SDL_SubmitGPUCommandBuffer(command_buffer))
      return utility::log("Could not submit GPU command buffer", utility::SDL_FAILURE);
    return EXIT_SUCCESS;
  }

  int Window::handle_quit()
  {
    running = false;
    return EXIT_SUCCESS;
  }

  int Window::handle_move()
  {
    if (fullscreen) return EXIT_SUCCESS;

    if (!SDL_GetWindowPosition(handle, &left, &top))
      return utility::log("Could not get window position", utility::SDL_FAILURE);
    display_index = SDL_GetDisplayForWindow(handle);
    if (display_index == 0) return utility::log("Could not get display for window", utility::SDL_FAILURE);

    return EXIT_SUCCESS;
  }

  int Window::handle_fullscreen()
  {
    if (fullscreen)
    {
      if (!SDL_SetWindowBordered(handle, true))
        return utility::log("Could not set window bordered", utility::SDL_FAILURE);
      if (!SDL_SetWindowSize(handle, starting_width, starting_height))
        return utility::log("Could not set window size", utility::SDL_FAILURE);
      if (!SDL_SetWindowPosition(handle, left, top))
        return utility::log("Could not set window position", utility::SDL_FAILURE);
    }
    else
    {
      SDL_Rect display_bounds;
      if (!SDL_GetDisplayBounds(display_index, &display_bounds))
        return utility::log("Could not get display bounds", utility::SDL_FAILURE);
      if (!SDL_SetWindowBordered(handle, false))
        return utility::log("Could not set window bordered", utility::SDL_FAILURE);
      if (!SDL_SetWindowSize(handle, display_bounds.w, display_bounds.h))
        return utility::log("Could not set window size", utility::SDL_FAILURE);
      if (!SDL_SetWindowPosition(handle, SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
                                 SDL_WINDOWPOS_CENTERED_DISPLAY(display_index)))
        return utility::log("Could not set window position", utility::SDL_FAILURE);
    }

    fullscreen = !fullscreen;
    return EXIT_SUCCESS;
  }

  void Window::update_simulation_time()
  {
    double current_simulation_time = static_cast<double>(SDL_GetTicksNS()) / 1e9;
    double delta_simulation_time = current_simulation_time - last_simulation_time;
    last_simulation_time = current_simulation_time;
    if (delta_simulation_time > 0.1) delta_simulation_time = 0.1;
    simulation_accumulator += delta_simulation_time;
  }

  bool Window::simulation_behind() { return simulation_accumulator >= target_simulation_time; }

  void Window::catchup_simulation() { simulation_accumulator -= target_simulation_time; }

  void Window::update_simulation_alpha() { simulation_alpha = simulation_accumulator / target_simulation_time; }

  bool Window::render_behind()
  {
    double current_render_time = static_cast<double>(SDL_GetTicksNS()) / 1e9;
    if (current_render_time - last_render_time >= target_render_time)
    {
      last_render_time = current_render_time;
      return true;
    }
    return false;
  }

  void Window::update_fps()
  {
    frame_count++;
    double current_fps_time = static_cast<double>(SDL_GetTicksNS()) / 1e9;
    if (current_fps_time - last_fps_time >= 1.0)
    {
      utility::log(std::to_string(frame_count) + " FPS", utility::TRACE);
      last_fps_time = current_fps_time;
      frame_count = 0;
    }
  }

  Window::Window(const std::string &title, bool i_fullscreen, int i_width, int i_height)
    : width(i_width), height(i_height), starting_width(i_width), starting_height(i_height)
  {
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
      utility::log("SDL could not be initialized", utility::SDL_FAILURE);
      return;
    }

    handle = SDL_CreateWindow(title.c_str(), i_width, i_height, SDL_WINDOW_HIDDEN);
    if (!handle)
    {
      utility::log("Could not create window", utility::SDL_FAILURE);
      return;
    }

    display_index = SDL_GetPrimaryDisplay();
    if (display_index == 0)
    {
      utility::log("Could not get primary display", utility::SDL_FAILURE);
      return;
    }
    left = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    top = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    if (!SDL_SetWindowPosition(handle, left, top))
    {
      utility::log("Could not set window position", utility::SDL_FAILURE);
      return;
    }
    if (i_fullscreen)
      if (handle_fullscreen() == SDL_APP_FAILURE) return;

    gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, true, nullptr);
    if (!gpu)
    {
      utility::log("Could not create GPU device", utility::SDL_FAILURE);
      return;
    }
    utility::log("Created GPU device " + std::string(SDL_GetGPUDeviceDriver(gpu)), utility::TRACE);
    if (!SDL_ClaimWindowForGPUDevice(gpu, handle))
    {
      utility::log("Could not claim window for GPU device", utility::SDL_FAILURE);
      return;
    }
    if (SDL_WindowSupportsGPUPresentMode(gpu, handle, SDL_GPU_PRESENTMODE_IMMEDIATE))
      if (!SDL_SetGPUSwapchainParameters(gpu, handle, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE))
      {
        utility::log("Could not set GPU swapchain parameters", utility::SDL_FAILURE);
        return;
      }

    SDL_ShowWindow(handle);
    running = true;
  }
}
