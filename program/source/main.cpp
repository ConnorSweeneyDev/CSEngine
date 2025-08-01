#include <algorithm>
#include <array>
#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <utility>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"

#include "camera.hpp"
#include "exception.hpp"
#include "game.hpp"
#include "object.hpp"
#include "resource.hpp"
#include "scene.hpp"
#include "window.hpp"

class custom_window : public cse::base::window
{
public:
  custom_window(const std::string &i_title, int i_starting_width, int i_starting_height, bool i_fullscreen,
                bool i_vsync)
    : window(i_title, i_starting_width, i_starting_height, i_fullscreen, i_vsync)
  {
  }

  void initialize() override
  {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata("CSEngine", "0.0.0", "Connor.Sweeney.Engine");
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
    if (fullscreen)
    {
      fullscreen = !fullscreen;
      handle_fullscreen();
    }

    gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, false, nullptr);
    if (!gpu) throw cse::utility::sdl_exception("Could not create GPU device for window {}", title);
    if (!SDL_ClaimWindowForGPUDevice(gpu, instance))
      throw cse::utility::sdl_exception("Could not claim window for GPU device for window {}", title);
    if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
      throw cse::utility::sdl_exception("Could not enable VSYNC for window {}", title);
    if (!vsync)
    {
      vsync = !vsync;
      handle_vsync();
    }

    SDL_ShowWindow(instance);
    running = true;
  }

  void cleanup() override
  {
    SDL_ReleaseWindowFromGPUDevice(gpu, instance);
    SDL_DestroyGPUDevice(gpu);
    SDL_DestroyWindow(instance);
    SDL_Quit();
  }

  void input() override
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

  bool start_render() override
  {
    command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    if (!command_buffer) throw cse::utility::sdl_exception("Could not acquire GPU command buffer");

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

    SDL_GPUViewport viewport = {};
    viewport.w = static_cast<float>(width);
    viewport.h = static_cast<float>(height);
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    SDL_SetGPUViewport(render_pass, &viewport);

    return true;
  }

  void end_render() override
  {
    SDL_EndGPURenderPass(render_pass);
    if (!SDL_SubmitGPUCommandBuffer(command_buffer))
      throw cse::utility::sdl_exception("Could not submit GPU command buffer");
  }
};

class custom_game : public cse::base::game
{
public:
  custom_game(std::unique_ptr<cse::base::window> custom_window) : cse::base::game(std::move(custom_window)) {}

  void initialize() override
  {
    window->initialize();
    if (scenes.empty()) throw cse::utility::exception("No scenes have been added to the game");
    if (!current_scene) throw cse::utility::exception("No current scene has been set for the game");
    current_scene->initialize(window->instance, window->gpu);
  }

  void cleanup() override
  {
    window->cleanup();
    current_scene->cleanup(window->gpu);
  }

  bool is_running() override { return window->running; }

  void input() override
  {
    window->input();
    current_scene->input(window->key_state);
  }

  void simulate() override { current_scene->simulate(simulation_alpha); }

  void render() override
  {
    if (!window->start_render()) return;
    current_scene->render(window->command_buffer, window->render_pass, window->width, window->height);
    window->end_render();
  }
};

class custom_scene : public cse::base::scene
{
public:
  custom_scene(std::unique_ptr<cse::base::camera> custom_camera) : cse::base::scene(std::move(custom_camera)) {}

  void initialize(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    for (const auto &object : objects) object.second->initialize(instance, gpu);
  }

  void cleanup(SDL_GPUDevice *gpu)
  {
    for (const auto &object : objects) object.second->cleanup(gpu);
  }

  void input(const bool *key_state)
  {
    camera->input(key_state);
    for (const auto &object : objects) object.second->input(key_state);
  }

  void simulate(double simulation_alpha)
  {
    camera->simulate(simulation_alpha);
    for (const auto &object : objects) object.second->simulate(simulation_alpha);
  }

  void render(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass, int width, int height)
  {
    camera->render(width, height);
    for (const auto &object : objects)
      object.second->render(command_buffer, render_pass, camera->graphics.projection_matrix,
                            camera->graphics.view_matrix);
  }
};

class custom_camera : public cse::base::camera
{
public:
  custom_camera(const glm::vec3 &starting_translation, const glm::vec3 &starting_forward, const glm::vec3 &starting_up,
                float starting_fov, float starting_near_clip, float starting_far_clip)
    : cse::base::camera(starting_translation, starting_forward, starting_up, starting_fov, starting_near_clip,
                        starting_far_clip)
  {
  }

  void input(const bool *key_state) override
  {
    if (key_state[SDL_SCANCODE_I]) transform.translation.acceleration.y += 0.0005f;
    if (key_state[SDL_SCANCODE_K]) transform.translation.acceleration.y -= 0.0005f;
    if (key_state[SDL_SCANCODE_L]) transform.translation.acceleration.x += 0.0005f;
    if (key_state[SDL_SCANCODE_J]) transform.translation.acceleration.x -= 0.0005f;
    if (key_state[SDL_SCANCODE_U]) transform.translation.acceleration.z -= 0.0005f;
    if (key_state[SDL_SCANCODE_O]) transform.translation.acceleration.z += 0.0005f;
  }

  void simulate(double simulation_alpha) override
  {
    transform.translation.previous = transform.translation.current;
    transform.translation.velocity += transform.translation.acceleration;
    for (int i = 0; i < 3; ++i)
    {
      transform.translation.acceleration[i] = -0.0001f;
      if (transform.translation.velocity[i] < 0.0f)
        transform.translation.velocity[i] -= transform.translation.acceleration[i];
      if (transform.translation.velocity[i] > 0.0f)
        transform.translation.velocity[i] += transform.translation.acceleration[i];
      if (transform.translation.velocity[i] < 0.0001f && transform.translation.velocity[i] > -0.0001f)
        transform.translation.velocity[i] = 0.0f;
    }
    transform.translation.acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
    transform.translation.current += transform.translation.velocity;
    transform.translation.interpolated =
      transform.translation.previous +
      ((transform.translation.current - transform.translation.previous) * static_cast<float>(simulation_alpha));
  }

  void render(int width, int height) override
  {
    graphics.projection_matrix =
      glm::perspective(glm::radians(fov), static_cast<float>(width) / static_cast<float>(height), near_clip, far_clip);
    graphics.view_matrix =
      glm::lookAt(transform.translation.interpolated,
                  transform.translation.interpolated + transform.forward.interpolated, transform.up.interpolated);
  }
};

class custom_object : public cse::base::object
{
public:
  custom_object(const glm::vec3 &starting_translation, const glm::vec3 &starting_rotation,
                const glm::vec3 &starting_scale, const cse::resource::compiled_shader &vertex_shader,
                const cse::resource::compiled_shader &fragment_shader)
    : cse::base::object(starting_translation, starting_rotation, starting_scale, vertex_shader, fragment_shader)
  {
  }

  void initialize(SDL_Window *instance, SDL_GPUDevice *gpu) override
  {
    SDL_GPUShaderFormat current_format = SDL_GPU_SHADERFORMAT_INVALID;
    const SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(gpu);
    if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV)
      current_format = SDL_GPU_SHADERFORMAT_SPIRV;
    else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL)
      current_format = SDL_GPU_SHADERFORMAT_DXIL;
    else
      throw cse::utility::sdl_exception("Could not find supported shader format for object");

    SDL_GPUShaderCreateInfo vertex_shader_info = {};
    vertex_shader_info.code = current_format == SDL_GPU_SHADERFORMAT_DXIL ? graphics.shader.vertex.dxil.data()
                                                                          : graphics.shader.vertex.spirv.data();
    vertex_shader_info.code_size = current_format == SDL_GPU_SHADERFORMAT_DXIL ? graphics.shader.vertex.dxil.size()
                                                                               : graphics.shader.vertex.spirv.size();
    vertex_shader_info.format = current_format;
    vertex_shader_info.entrypoint = "main";
    vertex_shader_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vertex_shader_info.num_samplers = 0;
    vertex_shader_info.num_uniform_buffers = 1;
    vertex_shader_info.num_storage_buffers = 0;
    vertex_shader_info.num_storage_textures = 0;
    SDL_GPUShader *vertex_shader = SDL_CreateGPUShader(gpu, &vertex_shader_info);
    if (!vertex_shader) throw cse::utility::sdl_exception("Could not create vertex shader for object");

    SDL_GPUShaderCreateInfo fragment_shader_info = {};
    fragment_shader_info.code = current_format == SDL_GPU_SHADERFORMAT_DXIL ? graphics.shader.fragment.dxil.data()
                                                                            : graphics.shader.fragment.spirv.data();
    fragment_shader_info.code_size = current_format == SDL_GPU_SHADERFORMAT_DXIL
                                       ? graphics.shader.fragment.dxil.size()
                                       : graphics.shader.fragment.spirv.size();
    fragment_shader_info.format = current_format;
    fragment_shader_info.entrypoint = "main";
    fragment_shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragment_shader_info.num_samplers = 0;
    fragment_shader_info.num_uniform_buffers = 0;
    fragment_shader_info.num_storage_buffers = 0;
    fragment_shader_info.num_storage_textures = 0;
    SDL_GPUShader *fragment_shader = SDL_CreateGPUShader(gpu, &fragment_shader_info);
    if (!fragment_shader) throw cse::utility::sdl_exception("Could not create fragment shader for object");

    SDL_GPUVertexInputState vertex_input = {};
    vertex_input.num_vertex_buffers = 1;
    vertex_input.num_vertex_attributes = 2;
    SDL_GPUVertexBufferDescription vertex_buffer_description = {};
    vertex_buffer_description.slot = 0;
    vertex_buffer_description.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertex_buffer_description.instance_step_rate = 0;
    vertex_buffer_description.pitch = sizeof(graphics::position_color_vertex);
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

    SDL_GPUColorTargetDescription color_target_description = {};
    color_target_description.format = SDL_GetGPUSwapchainTextureFormat(gpu, instance);

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.vertex_shader = vertex_shader;
    pipeline_info.fragment_shader = fragment_shader;
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    pipeline_info.vertex_input_state = vertex_input;
    pipeline_info.target_info.num_color_targets = 1;
    pipeline_info.target_info.color_target_descriptions = &color_target_description;
    graphics.pipeline = SDL_CreateGPUGraphicsPipeline(gpu, &pipeline_info);

    if (!graphics.pipeline) throw cse::utility::sdl_exception("Could not create graphics pipeline for object");
    SDL_ReleaseGPUShader(gpu, vertex_shader);
    SDL_ReleaseGPUShader(gpu, fragment_shader);

    SDL_GPUBufferCreateInfo vertex_buffer_info = {};
    vertex_buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertex_buffer_info.size = sizeof(graphics::position_color_vertex) * 4;
    graphics.vertex_buffer = SDL_CreateGPUBuffer(gpu, &vertex_buffer_info);
    if (!graphics.vertex_buffer) throw cse::utility::sdl_exception("Could not create vertex buffer for object");
    SDL_GPUBufferCreateInfo index_buffer_info = {};
    index_buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    index_buffer_info.size = sizeof(Uint16) * 6;
    graphics.index_buffer = SDL_CreateGPUBuffer(gpu, &index_buffer_info);
    if (!graphics.index_buffer) throw cse::utility::sdl_exception("Could not create index buffer for object");

    SDL_GPUTransferBufferCreateInfo transfer_buffer_info = {};
    transfer_buffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_buffer_info.size = (sizeof(graphics::position_color_vertex) * 4) + (sizeof(Uint16) * 6);
    SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &transfer_buffer_info);
    if (!transfer_buffer) throw cse::utility::sdl_exception("Could not create transfer buffer for object");
    auto vertex_data =
      reinterpret_cast<graphics::position_color_vertex *>(SDL_MapGPUTransferBuffer(gpu, transfer_buffer, false));
    if (!vertex_data) throw cse::utility::sdl_exception("Could not map vertex data for object");
    std::copy(graphics::default_quad_vertices.begin(), graphics::default_quad_vertices.end(), vertex_data);
    auto index_data = reinterpret_cast<Uint16 *>(&vertex_data[graphics::default_quad_vertices.size()]);
    if (!index_data) throw cse::utility::sdl_exception("Could not map index data for object");
    std::copy(graphics::default_quad_indices.begin(), graphics::default_quad_indices.end(), index_data);
    SDL_UnmapGPUTransferBuffer(gpu, transfer_buffer);

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    if (!command_buffer) throw cse::utility::sdl_exception("Could not acquire GPU command buffer for object");
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass) throw cse::utility::sdl_exception("Could not begin GPU copy pass for object");
    SDL_GPUTransferBufferLocation vertex_transfer_buffer_location = {};
    vertex_transfer_buffer_location.transfer_buffer = transfer_buffer;
    vertex_transfer_buffer_location.offset = 0;
    SDL_GPUBufferRegion vertex_buffer_region = {};
    vertex_buffer_region.buffer = graphics.vertex_buffer;
    vertex_buffer_region.offset = 0;
    vertex_buffer_region.size = sizeof(graphics::position_color_vertex) * 4;
    SDL_UploadToGPUBuffer(copy_pass, &vertex_transfer_buffer_location, &vertex_buffer_region, false);
    SDL_GPUTransferBufferLocation index_transfer_buffer_location = {};
    index_transfer_buffer_location.transfer_buffer = transfer_buffer;
    index_transfer_buffer_location.offset = sizeof(graphics::position_color_vertex) * 4;
    SDL_GPUBufferRegion index_buffer_region = {};
    index_buffer_region.buffer = graphics.index_buffer;
    index_buffer_region.offset = 0;
    index_buffer_region.size = sizeof(Uint16) * 6;
    SDL_UploadToGPUBuffer(copy_pass, &index_transfer_buffer_location, &index_buffer_region, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    SDL_ReleaseGPUTransferBuffer(gpu, transfer_buffer);
  }

  void cleanup(SDL_GPUDevice *gpu) override
  {
    SDL_ReleaseGPUBuffer(gpu, graphics.index_buffer);
    SDL_ReleaseGPUBuffer(gpu, graphics.vertex_buffer);
    SDL_ReleaseGPUGraphicsPipeline(gpu, graphics.pipeline);
  }

  void input(const bool *key_state) override
  {
    if (key_state[SDL_SCANCODE_E]) transform.translation.acceleration.y += 0.0005f;
    if (key_state[SDL_SCANCODE_D]) transform.translation.acceleration.y -= 0.0005f;
    if (key_state[SDL_SCANCODE_F]) transform.translation.acceleration.x += 0.0005f;
    if (key_state[SDL_SCANCODE_S]) transform.translation.acceleration.x -= 0.0005f;
    if (key_state[SDL_SCANCODE_W]) transform.translation.acceleration.z += 0.0005f;
    if (key_state[SDL_SCANCODE_R]) transform.translation.acceleration.z -= 0.0005f;
  }

  void simulate(double simulation_alpha) override
  {
    transform.translation.previous = transform.translation.current;
    transform.translation.velocity += transform.translation.acceleration;
    for (int i = 0; i < 3; ++i)
    {
      transform.translation.acceleration[i] = -0.0001f;
      if (transform.translation.velocity[i] < 0.0f)
        transform.translation.velocity[i] -= transform.translation.acceleration[i];
      if (transform.translation.velocity[i] > 0.0f)
        transform.translation.velocity[i] += transform.translation.acceleration[i];
      if (transform.translation.velocity[i] < 0.0001f && transform.translation.velocity[i] > -0.0001f)
        transform.translation.velocity[i] = 0.0f;
    }
    transform.translation.acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
    transform.translation.current += transform.translation.velocity;
    transform.translation.interpolated =
      transform.translation.previous +
      ((transform.translation.current - transform.translation.previous) * static_cast<float>(simulation_alpha));
  }

  void render(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass, const glm::mat4 &projection_matrix,
              const glm::mat4 &view_matrix) override
  {
    SDL_BindGPUGraphicsPipeline(render_pass, graphics.pipeline);
    SDL_GPUBufferBinding buffer_binding = {};
    buffer_binding.buffer = graphics.vertex_buffer;
    buffer_binding.offset = 0;
    SDL_BindGPUVertexBuffers(render_pass, 0, &buffer_binding, 1);
    buffer_binding.buffer = graphics.index_buffer;
    buffer_binding.offset = 0;
    SDL_BindGPUIndexBuffer(render_pass, &buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, transform.translation.interpolated);
    model_matrix =
      glm::rotate(model_matrix, glm::radians(transform.rotation.interpolated.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model_matrix =
      glm::rotate(model_matrix, glm::radians(transform.rotation.interpolated.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model_matrix =
      glm::rotate(model_matrix, glm::radians(transform.rotation.interpolated.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model_matrix = glm::scale(model_matrix, transform.scale.interpolated);
    std::array<glm::mat4, 3> matrices = {projection_matrix, view_matrix, model_matrix};
    SDL_PushGPUVertexUniformData(command_buffer, 0, &matrices, sizeof(matrices));

    SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
  }
};

int try_main(int argc, char *argv[])
{
  if (argc > 1 || !argv[0]) throw cse::utility::exception("Expected 1 argument, got {}", argc);

  {
    auto window = std::make_unique<custom_window>("CSE Example", 1280, 720, false, true);
    auto game = std::make_unique<custom_game>(std::move(window));
    auto camera = std::make_unique<custom_camera>(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                                                  glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, 0.01f, 10.0f);
    auto scene = std::make_unique<custom_scene>(std::move(camera));
    auto quad = std::make_unique<custom_object>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                                                glm::vec3(1.0f, 1.0f, 1.0f), cse::resource::main_vertex,
                                                cse::resource::main_fragment);
    scene->add_object("quad", std::move(quad));
    game->add_scene("scene", std::move(scene));
    game->set_current_scene("scene");
    game->run();
  }

  SDL_Log("Exiting application...");
  return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
  try
  {
    return try_main(argc, argv);
  }
  catch (const std::exception &error)
  {
    SDL_Log("%s", error.what());
    return EXIT_FAILURE;
  }
}
