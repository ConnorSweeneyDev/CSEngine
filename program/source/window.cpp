#include "window.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <memory>
#include <string>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"

#include "exception.hpp"

namespace cse
{
  std::unique_ptr<Window> Window::create(const std::string &i_title, bool i_fullscreen, int i_width, int i_height)
  {
    bool expected = false;
    if (!initialized.compare_exchange_strong(expected, true))
      throw Exception("A window already exists, could not create {}", i_title);
    return std::unique_ptr<Window>(new Window(i_title, i_fullscreen, i_width, i_height));
  }

  Window::~Window()
  {
    SDL_ReleaseWindowFromGPUDevice(gpu, window);
    SDL_DestroyGPUDevice(gpu);
    SDL_DestroyWindow(window);
    window = nullptr;
    running = false;
    SDL_Quit();
    initialized.store(false);
  }

  void Window::input()
  {
    SDL_Event event = {};
    const bool *key_state = SDL_GetKeyboardState(nullptr);
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
            default: break;
          }
        default: break;
      }
    }

    if (key_state[SDL_SCANCODE_D]) strength_acceleration -= 0.0005f;
    if (key_state[SDL_SCANCODE_F]) strength_acceleration += 0.0005f;
  }

  void Window::simulate()
  {
    previous_strength = current_strength;
    strength_velocity += strength_acceleration;
    if (!(strength_velocity < 0 && strength_velocity > 0))
    {
      strength_acceleration = -0.0001f;
      if (strength_velocity < 0) strength_velocity -= strength_acceleration;
      if (strength_velocity > 0) strength_velocity += strength_acceleration;
      if (strength_velocity < 0.0001f && strength_velocity > -0.0001f) strength_velocity = 0.0f;
    }
    current_strength += strength_velocity;
    if (current_strength < 0.0f)
    {
      current_strength = 0.0f;
      strength_velocity = 0.0f;
    }
    if (current_strength > 0.5f)
    {
      current_strength = 0.5f;
      strength_velocity = 0.0f;
    }
    strength_acceleration = 0.0f;
    interpolated_strength =
      previous_strength + ((current_strength - previous_strength) * static_cast<float>(simulation_alpha));
  }

  void Window::render()
  {
    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    if (!command_buffer) throw SDL_exception("Could not acquire GPU command buffer");

    SDL_GPUTexture *swapchain_texture = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr))
      throw SDL_exception("Could not acquire GPU swapchain texture");
    if (!swapchain_texture)
    {
      if (!SDL_SubmitGPUCommandBuffer(command_buffer)) throw SDL_exception("Could not submit GPU command buffer");
      return;
    }

    SDL_GPUColorTargetInfo color_target_info = {};
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;
    color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target_info.texture = swapchain_texture;
    color_target_info.clear_color = {interpolated_strength, interpolated_strength, interpolated_strength, 1.0f};
    SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr);
    if (!render_pass) throw SDL_exception("Could not begin GPU render pass");
    SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
    SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(render_pass);

    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) throw SDL_exception("Could not submit GPU command buffer");
  }

  void Window::handle_quit() { running = false; }

  void Window::handle_move()
  {
    if (fullscreen) return;

    if (!SDL_GetWindowPosition(window, &left, &top))
      throw SDL_exception("Could not get window position for window at ({}, {})", left, top);
    display_index = SDL_GetDisplayForWindow(window);
    if (display_index == 0) throw SDL_exception("Could not get display index");
  }

  void Window::handle_fullscreen()
  {
    if (fullscreen)
    {
      if (!SDL_SetWindowBordered(window, true)) throw SDL_exception("Could not set window bordered");
      if (!SDL_SetWindowSize(window, starting_width, starting_height))
        throw SDL_exception("Could not set window size to ({}, {})", starting_width, starting_height);
      if (!SDL_SetWindowPosition(window, left, top))
        throw SDL_exception("Could not set window position to ({}, {})", left, top);
    }
    else
    {
      SDL_Rect display_bounds;
      if (!SDL_GetDisplayBounds(display_index, &display_bounds))
        throw SDL_exception("Could not get display bounds for display {}", display_index);
      if (!SDL_SetWindowBordered(window, false)) throw SDL_exception("Could not set window borderless");
      if (!SDL_SetWindowSize(window, display_bounds.w, display_bounds.h))
        throw SDL_exception("Could not set window size to ({}, {}) on display {}", display_bounds.w, display_bounds.h,
                            display_index);
      if (!SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
                                 SDL_WINDOWPOS_CENTERED_DISPLAY(display_index)))
        throw SDL_exception("Could not set window position centered on display {}", display_index);
    }
    fullscreen = !fullscreen;
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
      SDL_Log("%d FPS", frame_count);
      last_fps_time = current_fps_time;
      frame_count = 0;
    }
  }

  Window::Window(const std::string &i_title, bool i_fullscreen, int i_width, int i_height)
    : width(i_width), height(i_height), starting_width(i_width), starting_height(i_height)
  {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata("CSEngine", "0.0.0", "Connor.Sweeney.Engine");
    if (!SDL_Init(SDL_INIT_VIDEO)) throw SDL_exception("SDL could not be initialized for window {}", i_title);

    window = SDL_CreateWindow(i_title.c_str(), i_width, i_height, SDL_WINDOW_HIDDEN);
    if (!window) throw SDL_exception("Could not create window {}", i_title);

    display_index = SDL_GetPrimaryDisplay();
    if (display_index == 0) throw SDL_exception("Could not get primary display for window {}", i_title);
    left = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    top = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    if (!SDL_SetWindowPosition(window, left, top))
      throw SDL_exception("Could not set window {} position to ({}, {})", i_title, left, top);
    if (i_fullscreen) handle_fullscreen();

    gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, true, nullptr);
    if (!gpu) throw SDL_exception("Could not create GPU device for window {}", i_title);
    if (!SDL_ClaimWindowForGPUDevice(gpu, window))
      throw SDL_exception("Could not claim window for GPU device for window {}", i_title);
    if (SDL_WindowSupportsGPUPresentMode(gpu, window, SDL_GPU_PRESENTMODE_IMMEDIATE))
      if (!SDL_SetGPUSwapchainParameters(gpu, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE))
        throw SDL_exception("Could not disable VSYNC for window {}", i_title);

    // START TODO: Refactor to load shaders from a generated resource.cpp file

    std::filesystem::path vertex_shader_path = "build/Shaders";
    std::filesystem::path fragment_shader_path = "build/Shaders";
    SDL_GPUShaderFormat shader_format = {};
    const SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(gpu);
    if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV)
    {
      vertex_shader_path /= "RawTriangle.vert.spv";
      fragment_shader_path /= "SolidColor.frag.spv";
      shader_format = SDL_GPU_SHADERFORMAT_SPIRV;
    }
    else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL)
    {
      vertex_shader_path /= "RawTriangle.vert.dxil";
      fragment_shader_path /= "SolidColor.frag.dxil";
      shader_format = SDL_GPU_SHADERFORMAT_DXIL;
    }
    else
      throw SDL_exception("Could not find supported shader format for window {}", i_title);

    std::ifstream vertex_shader_file(vertex_shader_path, std::ios::binary | std::ios::ate);
    if (!vertex_shader_file)
      throw SDL_exception("Could not open vertex shader file {} for window {}", vertex_shader_path.string(), i_title);
    std::streamsize vertex_shader_size = vertex_shader_file.tellg();
    vertex_shader_file.seekg(0, std::ios::beg);
    std::vector<Uint8> vertex_shader_binary(static_cast<size_t>(vertex_shader_size));
    if (!vertex_shader_file.read(reinterpret_cast<char *>(vertex_shader_binary.data()), vertex_shader_size))
      throw SDL_exception("Could not read vertex shader file {} for window {}", vertex_shader_path.string(), i_title);
    if (vertex_shader_binary.size() % 4 != 0)
      vertex_shader_binary.resize((vertex_shader_binary.size() + 3) & static_cast<size_t>(~3));
    vertex_shader_file.close();
    std::ifstream fragment_shader_file(fragment_shader_path, std::ios::binary | std::ios::ate);
    if (!fragment_shader_file)
      throw SDL_exception("Could not open fragment shader file {} for window {}", fragment_shader_path.string(),
                          i_title);
    std::streamsize fragment_shader_size = fragment_shader_file.tellg();
    fragment_shader_file.seekg(0, std::ios::beg);
    std::vector<Uint8> fragment_shader_binary(static_cast<size_t>(fragment_shader_size));
    if (!fragment_shader_file.read(reinterpret_cast<char *>(fragment_shader_binary.data()), fragment_shader_size))
      throw SDL_exception("Could not read fragment shader file {} for window {}", fragment_shader_path.string(),
                          i_title);
    if (fragment_shader_binary.size() % 4 != 0)
      fragment_shader_binary.resize((fragment_shader_binary.size() + 3) & static_cast<size_t>(~3));
    fragment_shader_file.close();

    SDL_GPUShaderCreateInfo shader_info = {};
    shader_info.code = vertex_shader_binary.data();
    shader_info.code_size = vertex_shader_binary.size();
    shader_info.entrypoint = "main";
    shader_info.format = shader_format;
    shader_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    shader_info.num_samplers = 0;
    shader_info.num_uniform_buffers = 0;
    shader_info.num_storage_buffers = 0;
    shader_info.num_storage_textures = 0;
    SDL_GPUShader *vertex_shader = SDL_CreateGPUShader(gpu, &shader_info);
    if (!vertex_shader) throw SDL_exception("Could not create vertex shader for window {}", i_title);
    shader_info.code = fragment_shader_binary.data();
    shader_info.code_size = fragment_shader_binary.size();
    shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    SDL_GPUShader *fragment_shader = SDL_CreateGPUShader(gpu, &shader_info);
    if (!fragment_shader) throw SDL_exception("Could not create fragment shader for window {}", i_title);

    SDL_GPUColorTargetDescription color_target_description = {};
    color_target_description.format = SDL_GetGPUSwapchainTextureFormat(gpu, window);
    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.vertex_shader = vertex_shader;
    pipeline_info.fragment_shader = fragment_shader;
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipeline_info.target_info.num_color_targets = 1;
    pipeline_info.target_info.color_target_descriptions = &color_target_description;
    pipeline = SDL_CreateGPUGraphicsPipeline(gpu, &pipeline_info);
    if (!pipeline) throw SDL_exception("Could not create graphics pipeline for window {}", i_title);

    SDL_ReleaseGPUShader(gpu, vertex_shader);
    SDL_ReleaseGPUShader(gpu, fragment_shader);

    // END TODO: Refactor to load shaders from a generated resource.cpp file

    SDL_ShowWindow(window);
    running = true;
  }
}
