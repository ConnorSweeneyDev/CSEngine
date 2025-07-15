#include "window.hpp"

#include <string>
#include <vector>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"

#include "utility.hpp"

namespace cse
{
  Window::Window(const std::string &title, int i_width, int i_height)
    : width(i_width), height(i_height), starting_width(i_width), starting_height(i_height)
  {
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
    if (fullscreen)
      if (handle_fullscreen() == SDL_APP_FAILURE) return;

    gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, true, nullptr);
    if (!gpu)
    {
      utility::log("Could not create GPU device", utility::SDL_FAILURE);
      return;
    }
    utility::log("Created GPU device " + std::string(SDL_GetGPUDeviceDriver(gpu)), utility::INFO);
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
    initialized = true;
  }

  Window::~Window()
  {
    if (!handle) return;

    SDL_DestroyWindow(handle);
    handle = nullptr;
    initialized = false;
  }

  SDL_AppResult Window::update()
  {
    Uint64 current_frame_time = SDL_GetTicksNS();
    delta_time = static_cast<double>(current_frame_time - past_frame_time) * 1e-9;

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    if (!command_buffer) return utility::log("Could not acquire GPU command buffer", utility::SDL_FAILURE);
    SDL_GPUTexture *swapchain_texture = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, handle, &swapchain_texture, nullptr, nullptr))
      return utility::log("Could not acquire GPU swapchain texture", utility::SDL_FAILURE);
    SDL_GPUColorTargetInfo color_target = {};
    color_target.store_op = SDL_GPU_STOREOP_STORE;
    color_target.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target.texture = swapchain_texture;
    color_target.clear_color = {0.1f, 0.1f, 0.1f, 1.0f};
    std::vector<SDL_GPUColorTargetInfo> color_targets = {color_target};
    SDL_GPURenderPass *render_pass =
      SDL_BeginGPURenderPass(command_buffer, color_targets.data(), static_cast<Uint32>(color_targets.size()), nullptr);
    if (!render_pass) return utility::log("Could not begin GPU render pass", utility::SDL_FAILURE);
    SDL_EndGPURenderPass(render_pass);
    if (!SDL_SubmitGPUCommandBuffer(command_buffer))
      return utility::log("Could not submit GPU command buffer", utility::SDL_FAILURE);

    if (current_frame_time - last_frame_time >= 1000000000)
    {
      last_frame_time = current_frame_time;
      utility::log(std::to_string(accumulated_frames) + " FPS", utility::INFO);
      accumulated_frames = 0;
    }
    past_frame_time = current_frame_time;
    accumulated_frames += 1;
    Uint64 elapsed = SDL_GetTicksNS() - current_frame_time;
    if (elapsed <= 1000000000 / target_frame_rate) SDL_DelayNS((1000000000 / target_frame_rate) - elapsed);

    return SDL_APP_CONTINUE;
  }

  SDL_AppResult Window::handle_move()
  {
    if (fullscreen) return SDL_APP_CONTINUE;

    if (!SDL_GetWindowPosition(handle, &left, &top))
      return utility::log("Could not get window position", utility::SDL_FAILURE);
    display_index = SDL_GetDisplayForWindow(handle);
    if (display_index == 0) return utility::log("Could not get display for window", utility::SDL_FAILURE);

    return SDL_APP_CONTINUE;
  }

  SDL_AppResult Window::handle_fullscreen()
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
    return SDL_APP_CONTINUE;
  }
}
