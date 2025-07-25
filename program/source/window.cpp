#include "window.hpp"

#include <algorithm>
#include <array>
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
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"

#include "exception.hpp"
#include "resource.hpp"

namespace cse
{
  std::unique_ptr<Window> Window::create(const std::string &i_title, int i_starting_width, int i_starting_height,
                                         bool i_fullscreen, bool i_vsync)
  {
    bool expected = false;
    if (!initialized.compare_exchange_strong(expected, true))
      throw Exception("A window already exists, could not create {}", i_title);
    return std::unique_ptr<Window>(new Window(i_title, i_starting_width, i_starting_height, i_fullscreen, i_vsync));
  }

  Window::~Window()
  {
    SDL_ReleaseGPUBuffer(gpu, vertex_buffer);
    SDL_ReleaseGPUBuffer(gpu, index_buffer);
    SDL_ReleaseGPUGraphicsPipeline(gpu, pipeline);
    SDL_ReleaseWindowFromGPUDevice(gpu, window);
    SDL_DestroyGPUDevice(gpu);
    SDL_DestroyWindow(window);
    SDL_Quit();
    running = false;
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
            case SDL_SCANCODE_F12: handle_vsync(); break;
            default: break;
          }
        default: break;
      }
    }

    if (key_state[SDL_SCANCODE_E]) view_translation_acceleration.y += 0.0005f;
    if (key_state[SDL_SCANCODE_D]) view_translation_acceleration.y -= 0.0005f;
    if (key_state[SDL_SCANCODE_F]) view_translation_acceleration.x += 0.0005f;
    if (key_state[SDL_SCANCODE_S]) view_translation_acceleration.x -= 0.0005f;
    if (key_state[SDL_SCANCODE_W]) view_translation_acceleration.z += 0.0005f;
    if (key_state[SDL_SCANCODE_R]) view_translation_acceleration.z -= 0.0005f;
  }

  void Window::simulate()
  {
    previous_view_translation = current_view_translation;
    view_translation_velocity += view_translation_acceleration;
    for (int i = 0; i < 3; ++i)
    {
      view_translation_acceleration[i] = -0.0001f;
      if (view_translation_velocity[i] < 0.0f) view_translation_velocity[i] -= view_translation_acceleration[i];
      if (view_translation_velocity[i] > 0.0f) view_translation_velocity[i] += view_translation_acceleration[i];
      if (view_translation_velocity[i] < 0.0001f && view_translation_velocity[i] > -0.0001f)
        view_translation_velocity[i] = 0.0f;
    }
    current_view_translation += view_translation_velocity;
    view_translation_acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
    interpolated_view_translation =
      previous_view_translation +
      ((current_view_translation - previous_view_translation) * static_cast<float>(simulation_alpha));
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

    // START TODO: Refactor

    SDL_GPUColorTargetInfo color_target_info = {};
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;
    color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target_info.texture = swapchain_texture;
    color_target_info.clear_color = {0.1f, 0.1f, 0.1f, 1.0f};
    SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr);
    if (!render_pass) throw SDL_exception("Could not begin GPU render pass");

    SDL_GPUViewport viewport = {};
    viewport.w = static_cast<float>(width);
    viewport.h = static_cast<float>(height);
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    SDL_SetGPUViewport(render_pass, &viewport);

    SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
    SDL_GPUBufferBinding buffer_binding = {};
    buffer_binding.buffer = vertex_buffer;
    buffer_binding.offset = 0;
    SDL_BindGPUVertexBuffers(render_pass, 0, &buffer_binding, 1);
    buffer_binding.buffer = index_buffer;
    buffer_binding.offset = 0;
    SDL_BindGPUIndexBuffer(render_pass, &buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    glm::mat4 projection_matrix =
      glm::perspective(glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 10.0f);
    // glm::vec3 view_translation = glm::vec3(0.0f, 0.0f, 2.0f);
    glm::vec3 view_direction = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 view_up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 view_matrix =
      glm::lookAt(interpolated_view_translation, interpolated_view_translation + view_direction, view_up);
    glm::vec3 model_translation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 model_rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 model_scale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, model_translation);
    model_matrix = glm::rotate(model_matrix, glm::radians(model_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model_matrix = glm::rotate(model_matrix, glm::radians(model_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model_matrix = glm::rotate(model_matrix, glm::radians(model_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model_matrix = glm::scale(model_matrix, model_scale);
    std::array<glm::mat4, 3> matrices = {projection_matrix, view_matrix, model_matrix};
    SDL_PushGPUVertexUniformData(command_buffer, 0, &matrices, sizeof(matrices));

    SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
    SDL_EndGPURenderPass(render_pass);

    // END TODO: Refactor

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
      width = starting_width;
      height = starting_height;
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
      width = display_bounds.w;
      height = display_bounds.h;
      if (!SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
                                 SDL_WINDOWPOS_CENTERED_DISPLAY(display_index)))
        throw SDL_exception("Could not set window position centered on display {}", display_index);
    }
    fullscreen = !fullscreen;
  }

  void Window::handle_vsync()
  {
    if (vsync)
    {
      if (SDL_WindowSupportsGPUPresentMode(gpu, window, SDL_GPU_PRESENTMODE_IMMEDIATE))
        if (!SDL_SetGPUSwapchainParameters(gpu, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                           SDL_GPU_PRESENTMODE_IMMEDIATE))
          throw SDL_exception("Could not disable VSYNC for window {}", title);
    }
    else if (!SDL_SetGPUSwapchainParameters(gpu, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
      throw SDL_exception("Could not enable VSYNC for window {}", title);
    vsync = !vsync;
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

  Window::Window(const std::string &i_title, int i_starting_width, int i_starting_height, bool i_fullscreen,
                 bool i_vsync)
    : starting_width(i_starting_width), starting_height(i_starting_height), width(i_starting_width),
      height(i_starting_height)
  {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata("CSEngine", "0.0.0", "Connor.Sweeney.Engine");
    if (!SDL_Init(SDL_INIT_VIDEO)) throw SDL_exception("SDL could not be initialized for window {}", i_title);

    window = SDL_CreateWindow(i_title.c_str(), i_starting_width, i_starting_height, SDL_WINDOW_HIDDEN);
    if (!window) throw SDL_exception("Could not create window {}", i_title);

    display_index = SDL_GetPrimaryDisplay();
    if (display_index == 0) throw SDL_exception("Could not get primary display for window {}", i_title);
    left = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    top = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    if (!SDL_SetWindowPosition(window, left, top))
      throw SDL_exception("Could not set window {} position to ({}, {})", i_title, left, top);
    if (i_fullscreen) handle_fullscreen();

    gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, false, nullptr);
    if (!gpu) throw SDL_exception("Could not create GPU device for window {}", i_title);
    if (!SDL_ClaimWindowForGPUDevice(gpu, window))
      throw SDL_exception("Could not claim window for GPU device for window {}", i_title);
    if (!SDL_SetGPUSwapchainParameters(gpu, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
      throw SDL_exception("Could not enable VSYNC for window {}", title);
    if (!i_vsync) handle_vsync();

    // START TODO: Refactor

    SDL_GPUShaderCreateInfo shader_info = {};
    const SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(gpu);
    if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV)
    {
      shader_info.code = resource::main_vertex.spirv.data();
      shader_info.code_size = resource::main_vertex.spirv.size();
      shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    }
    else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL)
    {
      shader_info.code = resource::main_vertex.dxil.data();
      shader_info.code_size = resource::main_vertex.dxil.size();
      shader_info.format = SDL_GPU_SHADERFORMAT_DXIL;
    }
    else
      throw SDL_exception("Could not find supported shader format for window {}", i_title);
    shader_info.entrypoint = "main";
    shader_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    shader_info.num_samplers = 0;
    shader_info.num_uniform_buffers = 1;
    shader_info.num_storage_buffers = 0;
    shader_info.num_storage_textures = 0;
    SDL_GPUShader *vertex_shader = SDL_CreateGPUShader(gpu, &shader_info);
    if (!vertex_shader) throw SDL_exception("Could not create vertex shader for window {}", i_title);
    if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV)
    {
      shader_info.code = resource::main_fragment.spirv.data();
      shader_info.code_size = resource::main_fragment.spirv.size();
      shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    }
    else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL)
    {
      shader_info.code = resource::main_fragment.dxil.data();
      shader_info.code_size = resource::main_fragment.dxil.size();
      shader_info.format = SDL_GPU_SHADERFORMAT_DXIL;
    }
    else
      throw SDL_exception("Could not find supported shader format for window {}", i_title);
    shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    shader_info.num_uniform_buffers = 0;
    SDL_GPUShader *fragment_shader = SDL_CreateGPUShader(gpu, &shader_info);
    if (!fragment_shader) throw SDL_exception("Could not create fragment shader for window {}", i_title);

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.vertex_shader = vertex_shader;
    pipeline_info.fragment_shader = fragment_shader;
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;

    SDL_GPUVertexInputState vertex_input = {};
    vertex_input.num_vertex_buffers = 1;
    vertex_input.num_vertex_attributes = 2;
    SDL_GPUVertexBufferDescription vertex_buffer_description = {};
    vertex_buffer_description.slot = 0;
    vertex_buffer_description.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertex_buffer_description.instance_step_rate = 0;
    vertex_buffer_description.pitch = sizeof(Position_color_vertex);
    vertex_input.vertex_buffer_descriptions = &vertex_buffer_description;
    std::array<SDL_GPUVertexAttribute, 2> vertex_attributes;
    vertex_attributes.at(0).buffer_slot = 0;
    vertex_attributes.at(0).format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertex_attributes.at(0).location = 0;
    vertex_attributes.at(0).offset = 0;
    vertex_attributes.at(1).buffer_slot = 0;
    vertex_attributes.at(1).format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
    vertex_attributes.at(1).location = 1;
    vertex_attributes.at(1).offset = sizeof(float) * 3;
    vertex_input.vertex_attributes = vertex_attributes.data();
    pipeline_info.vertex_input_state = vertex_input;

    pipeline_info.target_info.num_color_targets = 1;
    SDL_GPUColorTargetDescription color_target_description = {};
    color_target_description.format = SDL_GetGPUSwapchainTextureFormat(gpu, window);
    pipeline_info.target_info.color_target_descriptions = &color_target_description;

    pipeline = SDL_CreateGPUGraphicsPipeline(gpu, &pipeline_info);
    if (!pipeline) throw SDL_exception("Could not create graphics pipeline for window {}", i_title);
    SDL_ReleaseGPUShader(gpu, vertex_shader);
    SDL_ReleaseGPUShader(gpu, fragment_shader);

    SDL_GPUBufferCreateInfo buffer_info = {};
    buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    buffer_info.size = sizeof(Position_color_vertex) * 4;
    vertex_buffer = SDL_CreateGPUBuffer(gpu, &buffer_info);
    if (!vertex_buffer) throw SDL_exception("Could not create vertex buffer for window {}", i_title);
    buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    buffer_info.size = sizeof(Uint16) * 6;
    index_buffer = SDL_CreateGPUBuffer(gpu, &buffer_info);
    if (!index_buffer) throw SDL_exception("Could not create index buffer for window {}", i_title);

    SDL_GPUTransferBufferCreateInfo transfer_buffer_info = {};
    transfer_buffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_buffer_info.size = (sizeof(Position_color_vertex) * 4) + (sizeof(Uint16) * 6);
    SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &transfer_buffer_info);
    if (!transfer_buffer) throw SDL_exception("Could not create transfer buffer for window {}", i_title);

    auto vertex_data = reinterpret_cast<Position_color_vertex *>(SDL_MapGPUTransferBuffer(gpu, transfer_buffer, false));
    if (!vertex_data) throw SDL_exception("Could not map vertex data for window {}", i_title);
    std::copy(default_quad_vertices.begin(), default_quad_vertices.end(), vertex_data);
    auto index_data = reinterpret_cast<Uint16 *>(&vertex_data[default_quad_vertices.size()]);
    if (!index_data) throw SDL_exception("Could not map index data for window {}", i_title);
    std::copy(default_quad_indices.begin(), default_quad_indices.end(), index_data);
    SDL_UnmapGPUTransferBuffer(gpu, transfer_buffer);

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    if (!command_buffer) throw SDL_exception("Could not acquire GPU command buffer for window {}", i_title);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass) throw SDL_exception("Could not begin GPU copy pass for window {}", i_title);

    SDL_GPUTransferBufferLocation transfer_buffer_location = {};
    transfer_buffer_location.transfer_buffer = transfer_buffer;
    transfer_buffer_location.offset = 0;
    SDL_GPUBufferRegion buffer_region = {};
    buffer_region.buffer = vertex_buffer;
    buffer_region.offset = 0;
    buffer_region.size = sizeof(Position_color_vertex) * 4;
    SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, false);
    transfer_buffer_location.offset = sizeof(Position_color_vertex) * 4;
    buffer_region.buffer = index_buffer;
    buffer_region.size = sizeof(Uint16) * 6;
    SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    SDL_ReleaseGPUTransferBuffer(gpu, transfer_buffer);

    // END TODO: Refactor

    SDL_ShowWindow(window);
    running = true;
  }
}
