#include "graphics.hpp"

#include <algorithm>
#include <array>
#include <string>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "glm/ext/vector_uint4_sized.hpp"
#include "glm/trigonometric.hpp"

#include "exception.hpp"
#include "resource.hpp"

namespace cse::helper
{
  window_graphics::~window_graphics()
  {
    render_pass = nullptr;
    depth_texture = nullptr;
    swapchain_texture = nullptr;
    command_buffer = nullptr;
    gpu = nullptr;
    instance = nullptr;
  }

  void window_graphics::create_app_and_window(const std::string &title, const unsigned int width,
                                              const unsigned int height, int &left, int &top,
                                              SDL_DisplayID &display_index, const bool fullscreen, const bool vsync)
  {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game"))
      throw cse::utility::sdl_exception("Could not set app metadata type");
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING, "Connor.Sweeney.Engine"))
      throw cse::utility::sdl_exception("Could not set app metadata identifier");
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "CSEngine"))
      throw cse::utility::sdl_exception("Could not set app metadata name");
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, "0.0.0"))
      throw cse::utility::sdl_exception("Could not set app metadata version");
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "Connor Sweeney"))
      throw cse::utility::sdl_exception("Could not set app metadata creator");
    if (!SDL_Init(SDL_INIT_VIDEO)) throw cse::utility::sdl_exception("SDL could not be initialized");

    instance = SDL_CreateWindow(title.c_str(), static_cast<int>(width), static_cast<int>(height),
                                SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
    if (!instance) throw cse::utility::sdl_exception("Could not create window");
    windowed_width = width;
    windowed_height = height;
    display_index = SDL_GetPrimaryDisplay();
    if (display_index == 0) throw cse::utility::sdl_exception("Could not get primary display");
    auto position = calculate_display_center(display_index, width, height);
    left = static_cast<int>(position.x);
    top = static_cast<int>(position.y);
    windowed_left = left;
    windowed_top = top;
    if (!SDL_SetWindowPosition(instance, left, top))
      throw cse::utility::sdl_exception("Could not set window position to {}, {}", left, top);
    gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
    if (!gpu) throw cse::utility::sdl_exception("Could not create GPU device");
    if (!SDL_ClaimWindowForGPUDevice(gpu, instance))
      throw cse::utility::sdl_exception("Could not claim window for GPU device");
    if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
      throw cse::utility::sdl_exception("Could not enable VSYNC");
    if (fullscreen) handle_fullscreen(fullscreen, left, top, display_index);
    if (!vsync) handle_vsync(vsync);
    if (!depth_texture) generate_depth_texture(width, height);
    SDL_ShowWindow(instance);
  }

  bool window_graphics::acquire_swapchain_texture()
  {
    command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    if (!command_buffer) throw cse::utility::sdl_exception("Could not acquire GPU command buffer");
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, instance, &swapchain_texture, nullptr, nullptr))
      throw cse::utility::sdl_exception("Could not acquire GPU swapchain texture");
    if (!swapchain_texture)
    {
      if (!SDL_SubmitGPUCommandBuffer(command_buffer))
        throw cse::utility::sdl_exception("Could not submit GPU command buffer");
      return false;
    }
    return true;
  }

  void window_graphics::start_render_pass(const float target_aspect_ratio, const unsigned int width,
                                          const unsigned int height)
  {
    SDL_GPUColorTargetInfo color_target_info = {};
    color_target_info.texture = swapchain_texture;
    color_target_info.clear_color = {0.1f, 0.1f, 0.1f, 1.0f};
    color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPUDepthStencilTargetInfo depth_stencil_target_info = {};
    depth_stencil_target_info.texture = depth_texture;
    depth_stencil_target_info.clear_depth = 1.0f;
    depth_stencil_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_stencil_target_info.store_op = SDL_GPU_STOREOP_STORE;
    render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, &depth_stencil_target_info);
    if (!render_pass) throw cse::utility::sdl_exception("Could not begin GPU render pass");
    float viewport_x = {}, viewport_y = {}, viewport_width = {}, viewport_height = {};
    if ((static_cast<float>(width) / static_cast<float>(height)) > target_aspect_ratio)
    {
      viewport_height = static_cast<float>(height);
      viewport_width = viewport_height * target_aspect_ratio;
      viewport_y = 0.0f;
      viewport_x = (static_cast<float>(width) - viewport_width) / 2.0f;
    }
    else
    {
      viewport_width = static_cast<float>(width);
      viewport_height = viewport_width / target_aspect_ratio;
      viewport_x = 0.0f;
      viewport_y = (static_cast<float>(height) - viewport_height) / 2.0f;
    }
    SDL_GPUViewport viewport = {.x = viewport_x,
                                .y = viewport_y,
                                .w = viewport_width,
                                .h = viewport_height,
                                .min_depth = 0.0f,
                                .max_depth = 1.0f};
    SDL_SetGPUViewport(render_pass, &viewport);
  }

  void window_graphics::end_render_pass()
  {
    SDL_EndGPURenderPass(render_pass);
    if (!SDL_SubmitGPUCommandBuffer(command_buffer))
      throw cse::utility::sdl_exception("Could not submit GPU command buffer");
  }

  void window_graphics::generate_depth_texture(const unsigned int width, const unsigned int height)
  {
    if (depth_texture)
    {
      SDL_ReleaseGPUTexture(gpu, depth_texture);
      depth_texture = nullptr;
    }
    SDL_GPUTextureCreateInfo depth_texture_info = {.type = SDL_GPU_TEXTURETYPE_2D,
                                                   .format = SDL_GPU_TEXTUREFORMAT_D24_UNORM,
                                                   .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
                                                   .width = width,
                                                   .height = height,
                                                   .layer_count_or_depth = 1,
                                                   .num_levels = 1,
                                                   .sample_count = SDL_GPU_SAMPLECOUNT_1,
                                                   .props = 0};
    depth_texture = SDL_CreateGPUTexture(gpu, &depth_texture_info);
    if (!depth_texture) throw cse::utility::sdl_exception("Could not create depth texture");
  }

  glm::uvec2 window_graphics::calculate_display_center(const SDL_DisplayID display_index, const unsigned int width,
                                                       const unsigned int height)
  {
    SDL_Rect display_bounds = {};
    if (!SDL_GetDisplayBounds(display_index, &display_bounds))
      throw utility::sdl_exception("Could not get bounds for display {}", display_index);
    return {display_bounds.x + (display_bounds.w - static_cast<int>(width)) / 2,
            display_bounds.y + (display_bounds.h - static_cast<int>(height)) / 2};
  }

  void window_graphics::handle_move(int &left, int &top, SDL_DisplayID &display_index, const bool fullscreen)
  {
    if (fullscreen) return;
    if (!SDL_GetWindowPosition(instance, &left, &top)) throw utility::sdl_exception("Could not get window position");
    windowed_left = left;
    windowed_top = top;
    display_index = SDL_GetDisplayForWindow(instance);
    if (display_index == 0) throw utility::sdl_exception("Could not get window display index");
  }

  void window_graphics::handle_manual_move(const int left, const int top, const bool fullscreen)
  {
    if (!fullscreen)
    {
      windowed_left = left;
      windowed_top = top;
    }
    if (!SDL_SetWindowPosition(instance, left, top))
      throw utility::sdl_exception("Could not set window position to {}, {}", left, top);
  }

  void window_graphics::handle_manual_display_move(const unsigned int width, const unsigned int height, int &left,
                                                   int &top, const SDL_DisplayID display_index, const bool fullscreen)
  {
    auto position = calculate_display_center(display_index, width, height);
    left = static_cast<int>(position.x);
    top = static_cast<int>(position.y);
    if (!fullscreen)
    {
      windowed_left = left;
      windowed_top = top;
    }
    if (!SDL_SetWindowPosition(instance, left, top))
      throw utility::sdl_exception("Could not set window position centered on display {}", display_index);
  }

  void window_graphics::handle_resize(unsigned int &width, unsigned int &height, SDL_DisplayID &display_index,
                                      const bool fullscreen)
  {
    SDL_GetWindowSize(instance, reinterpret_cast<int *>(&width), reinterpret_cast<int *>(&height));
    if (!fullscreen)
    {
      windowed_width = width;
      windowed_height = height;
    }
    display_index = SDL_GetDisplayForWindow(instance);
    generate_depth_texture(width, height);
  }

  void window_graphics::handle_manual_resize(const unsigned int width, const unsigned int height, const bool fullscreen)
  {
    if (!fullscreen)
    {
      windowed_width = width;
      windowed_height = height;
    }
    if (!SDL_SetWindowSize(instance, static_cast<int>(width), static_cast<int>(height)))
      throw utility::sdl_exception("Could not set window size to {}, {}", width, height);
    generate_depth_texture(width, height);
  }

  void window_graphics::handle_fullscreen(const bool fullscreen, int &left, int &top, const SDL_DisplayID display_index)
  {
    if (fullscreen)
    {
      SDL_Rect display_bounds = {};
      if (!SDL_GetDisplayBounds(display_index, &display_bounds))
        throw utility::sdl_exception("Could not get bounds for display {}", display_index);
      if (!SDL_SetWindowBordered(instance, false)) throw utility::sdl_exception("Could not set window borderless");
      if (!SDL_SetWindowSize(instance, display_bounds.w, display_bounds.h))
        throw utility::sdl_exception("Could not set window size to {}, {} on display {}", display_bounds.w,
                                     display_bounds.h, display_index);
      auto position = calculate_display_center(display_index, static_cast<unsigned int>(display_bounds.w),
                                               static_cast<unsigned int>(display_bounds.h));
      left = static_cast<int>(position.x);
      top = static_cast<int>(position.y);
      if (!SDL_SetWindowPosition(instance, left, top))
        throw utility::sdl_exception("Could not set window position centered on display {}", display_index);
      return;
    }
    if (!SDL_SetWindowBordered(instance, true)) throw utility::sdl_exception("Could not set window bordered");
    if (!SDL_SetWindowSize(instance, static_cast<int>(windowed_width), static_cast<int>(windowed_height)))
      throw utility::sdl_exception("Could not set window size to {}, {}", windowed_width, windowed_height);
    if (!SDL_SetWindowPosition(instance, windowed_left, windowed_top))
      throw utility::sdl_exception("Could not set window position to {}, {}", left, top);
  }

  void window_graphics::handle_vsync(const bool vsync)
  {
    if (vsync)
    {
      if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
        throw utility::sdl_exception("Could not enable VSYNC");
      return;
    }
    if (SDL_WindowSupportsGPUPresentMode(gpu, instance, SDL_GPU_PRESENTMODE_IMMEDIATE))
      if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                         SDL_GPU_PRESENTMODE_IMMEDIATE))
        throw utility::sdl_exception("Could not disable VSYNC");
  }

  void window_graphics::destroy_window_and_app()
  {
    SDL_ReleaseGPUTexture(gpu, depth_texture);
    SDL_ReleaseWindowFromGPUDevice(gpu, instance);
    SDL_DestroyGPUDevice(gpu);
    SDL_DestroyWindow(instance);
    SDL_Quit();
  }

  camera_graphics::camera_graphics(const float fov_) : fov(fov_), near_clip(0.01f), far_clip(100.0f) {}

  glm::mat4 camera_graphics::calculate_projection_matrix(const float target_aspect_ratio)
  {
    return glm::perspective(glm::radians(fov), target_aspect_ratio, near_clip, far_clip);
  }

  object_graphics::object_graphics(const glm::u8vec4 &tint_, const resource::compiled_shader &vertex_shader_,
                                   const resource::compiled_shader &fragment_shader_,
                                   const resource::compiled_texture &texture_, const std::string &frame_group_)
    : tint(tint_), shader(vertex_shader_, fragment_shader_), texture(texture_, frame_group_)
  {
  }

  object_graphics::~object_graphics()
  {
    texture_transfer_buffer = nullptr;
    vertex_transfer_buffer = nullptr;
    sampler_buffer = nullptr;
    texture_buffer = nullptr;
    index_buffer = nullptr;
    vertex_buffer = nullptr;
    pipeline = nullptr;
  }

  void object_graphics::create_pipeline_and_buffers(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    const SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(gpu);
    if (!(backend_formats & SDL_GPU_SHADERFORMAT_SPIRV))
      throw cse::utility::sdl_exception("No supported vulkan shader formats for object");
    SDL_GPUShaderCreateInfo vertex_shader_info = {.code_size = shader.vertex.length,
                                                  .code = shader.vertex.source.data(),
                                                  .entrypoint = "main",
                                                  .format = SDL_GPU_SHADERFORMAT_SPIRV,
                                                  .stage = SDL_GPU_SHADERSTAGE_VERTEX,
                                                  .num_samplers = 0,
                                                  .num_storage_textures = 0,
                                                  .num_storage_buffers = 0,
                                                  .num_uniform_buffers = 1,
                                                  .props = 0};
    SDL_GPUShader *vertex_shader = SDL_CreateGPUShader(gpu, &vertex_shader_info);
    if (!vertex_shader) throw cse::utility::sdl_exception("Could not create vertex shader for object");
    SDL_GPUShaderCreateInfo fragment_shader_info = {.code_size = shader.fragment.length,
                                                    .code = shader.fragment.source.data(),
                                                    .entrypoint = "main",
                                                    .format = SDL_GPU_SHADERFORMAT_SPIRV,
                                                    .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
                                                    .num_samplers = 1,
                                                    .num_storage_textures = 0,
                                                    .num_storage_buffers = 0,
                                                    .num_uniform_buffers = 0,
                                                    .props = 0};
    SDL_GPUShader *fragment_shader = SDL_CreateGPUShader(gpu, &fragment_shader_info);
    if (!fragment_shader) throw cse::utility::sdl_exception("Could not create fragment shader for object");
    SDL_GPUVertexBufferDescription vertex_buffer_description = {
      .slot = 0, .pitch = sizeof(vertex), .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX, .instance_step_rate = 0};
    std::array<SDL_GPUVertexAttribute, 3> vertex_attributes(
      {{0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0},
       {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, sizeof(vertex::x) + sizeof(vertex::y) + sizeof(vertex::z)},
       {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
        sizeof(vertex::x) + sizeof(vertex::y) + sizeof(vertex::z) + sizeof(vertex::r) + sizeof(vertex::g) +
          sizeof(vertex::b) + sizeof(vertex::a)}});
    SDL_GPUVertexInputState vertex_input_state = {.vertex_buffer_descriptions = &vertex_buffer_description,
                                                  .num_vertex_buffers = 1,
                                                  .vertex_attributes = vertex_attributes.data(),
                                                  .num_vertex_attributes = 3};
    SDL_GPURasterizerState rasterizer_state = {};
    rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    SDL_GPUColorTargetDescription color_target_description = {};
    color_target_description.format = SDL_GetGPUSwapchainTextureFormat(gpu, instance);
    SDL_GPUDepthStencilState depth_stencil_state = {};
    depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    depth_stencil_state.enable_depth_test = true;
    depth_stencil_state.enable_depth_write = true;
    SDL_GPUGraphicsPipelineTargetInfo target_info = {};
    target_info.color_target_descriptions = &color_target_description;
    target_info.num_color_targets = 1;
    target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM;
    target_info.has_depth_stencil_target = true;
    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.vertex_shader = vertex_shader;
    pipeline_info.fragment_shader = fragment_shader;
    pipeline_info.vertex_input_state = vertex_input_state;
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_info.rasterizer_state = rasterizer_state;
    pipeline_info.depth_stencil_state = depth_stencil_state;
    pipeline_info.target_info = target_info;
    pipeline = SDL_CreateGPUGraphicsPipeline(gpu, &pipeline_info);
    if (!pipeline) throw cse::utility::sdl_exception("Could not create graphics pipeline for object");
    SDL_ReleaseGPUShader(gpu, fragment_shader);
    SDL_ReleaseGPUShader(gpu, vertex_shader);

    SDL_GPUBufferCreateInfo vertex_buffer_info = {
      .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = sizeof(quad_vertices), .props = 0};
    vertex_buffer = SDL_CreateGPUBuffer(gpu, &vertex_buffer_info);
    if (!vertex_buffer) throw cse::utility::sdl_exception("Could not create vertex buffer for object");
    SDL_GPUBufferCreateInfo index_buffer_info = {
      .usage = SDL_GPU_BUFFERUSAGE_INDEX, .size = sizeof(quad_indices), .props = 0};
    index_buffer = SDL_CreateGPUBuffer(gpu, &index_buffer_info);
    if (!index_buffer) throw cse::utility::sdl_exception("Could not create index buffer for object");
    SDL_GPUSamplerCreateInfo sampler_info = {};
    sampler_info.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_buffer = SDL_CreateGPUSampler(gpu, &sampler_info);
    if (!sampler_buffer) throw cse::utility::sdl_exception("Could not create sampler for object");
    SDL_GPUTextureCreateInfo texture_info = {.type = SDL_GPU_TEXTURETYPE_2D,
                                             .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                             .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
                                             .width = texture.data.image_data.width,
                                             .height = texture.data.image_data.height,
                                             .layer_count_or_depth = 1,
                                             .num_levels = 1,
                                             .sample_count = SDL_GPU_SAMPLECOUNT_1,
                                             .props = 0};
    texture_buffer = SDL_CreateGPUTexture(gpu, &texture_info);
    if (!texture_buffer) throw cse::utility::sdl_exception("Could not create texture for object");

    SDL_GPUTransferBufferCreateInfo vertex_transfer_buffer_info = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = sizeof(quad_vertices) + sizeof(quad_indices), .props = 0};
    vertex_transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &vertex_transfer_buffer_info);
    if (!vertex_transfer_buffer)
      throw cse::utility::sdl_exception("Could not create transfer buffer for buffer object");
    auto vertex_data = reinterpret_cast<vertex *>(SDL_MapGPUTransferBuffer(gpu, vertex_transfer_buffer, false));
    if (!vertex_data) throw cse::utility::sdl_exception("Could not map vertex data for object");
    quad_vertices = std::array<vertex, 4>({{1.0f, 1.0f, 0.0f, 0, 0, 0, 0, 1.0f, 1.0f},
                                           {1.0f, -1.0f, 0.0f, 0, 0, 0, 0, 1.0f, 0.0f},
                                           {-1.0f, 1.0f, 0.0f, 0, 0, 0, 0, 0.0f, 1.0f},
                                           {-1.0f, -1.0f, 0.0f, 0, 0, 0, 0, 0.0f, 0.0f}});
    std::copy(quad_vertices.begin(), quad_vertices.end(), vertex_data);
    auto index_data = reinterpret_cast<Uint16 *>(&vertex_data[quad_vertices.size()]);
    if (!index_data) throw cse::utility::sdl_exception("Could not map index data for object");
    quad_indices = std::array<Uint16, 6>({3, 1, 0, 3, 0, 2});
    std::copy(quad_indices.begin(), quad_indices.end(), index_data);
    SDL_UnmapGPUTransferBuffer(gpu, vertex_transfer_buffer);
    SDL_GPUTransferBufferCreateInfo texture_transfer_buffer_info = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = texture.data.image_data.width * texture.data.image_data.height * texture.data.image_data.channels,
      .props = 0};
    texture_transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &texture_transfer_buffer_info);
    if (!texture_transfer_buffer)
      throw cse::utility::sdl_exception("Could not create transfer buffer for texture for object");
    auto *texture_data = reinterpret_cast<Uint8 *>(SDL_MapGPUTransferBuffer(gpu, texture_transfer_buffer, false));
    if (!texture_data) throw cse::utility::sdl_exception("Could not map texture data for object");
    SDL_memcpy(texture_data, texture.data.image.data(),
               texture.data.image_data.width * texture.data.image_data.height * texture.data.image_data.channels);
    SDL_UnmapGPUTransferBuffer(gpu, texture_transfer_buffer);
  }

  void object_graphics::upload_static_buffers(SDL_GPUDevice *gpu)
  {
    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    if (!command_buffer) throw cse::utility::sdl_exception("Could not acquire GPU command buffer for object");
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass) throw cse::utility::sdl_exception("Could not begin GPU copy pass for object");
    SDL_GPUTransferBufferLocation index_transfer_buffer_location{.transfer_buffer = vertex_transfer_buffer,
                                                                 .offset = sizeof(quad_vertices)};
    SDL_GPUBufferRegion index_buffer_region = {.buffer = index_buffer, .offset = 0, .size = sizeof(quad_indices)};
    SDL_UploadToGPUBuffer(copy_pass, &index_transfer_buffer_location, &index_buffer_region, false);
    SDL_GPUTextureTransferInfo texture_transfer_info = {
      .transfer_buffer = texture_transfer_buffer, .offset = 0, .pixels_per_row = 0, .rows_per_layer = 0};
    SDL_GPUTextureRegion texture_region = {.texture = texture_buffer,
                                           .mip_level = 0,
                                           .layer = 0,
                                           .x = 0,
                                           .y = 0,
                                           .z = 0,
                                           .w = texture.data.image_data.width,
                                           .h = texture.data.image_data.height,
                                           .d = 1};
    SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    SDL_ReleaseGPUTransferBuffer(gpu, texture_transfer_buffer);
    texture_transfer_buffer = nullptr;
  }

  void object_graphics::upload_dynamic_buffers(SDL_GPUDevice *gpu)
  {
    auto vertex_data = reinterpret_cast<vertex *>(SDL_MapGPUTransferBuffer(gpu, vertex_transfer_buffer, false));
    if (!vertex_data) throw cse::utility::sdl_exception("Could not map vertex data for object");
    const auto &frame_coords =
      texture.data.frame_data.find_group(texture.frame_group).frames[texture.frame_index].coords;
    quad_vertices = std::array<vertex, 4>(
      {{1.0f, 1.0f, 0.0f, tint.r, tint.g, tint.b, tint.a, frame_coords.right, frame_coords.top},
       {1.0f, -1.0f, 0.0f, tint.r, tint.g, tint.b, tint.a, frame_coords.right, frame_coords.bottom},
       {-1.0f, 1.0f, 0.0f, tint.r, tint.g, tint.b, tint.a, frame_coords.left, frame_coords.top},
       {-1.0f, -1.0f, 0.0f, tint.r, tint.g, tint.b, tint.a, frame_coords.left, frame_coords.bottom}});
    std::copy(quad_vertices.begin(), quad_vertices.end(), vertex_data);
    SDL_UnmapGPUTransferBuffer(gpu, vertex_transfer_buffer);
    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    if (!command_buffer) throw cse::utility::sdl_exception("Could not acquire GPU command buffer for object");
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass) throw cse::utility::sdl_exception("Could not begin GPU copy pass for object");
    SDL_GPUTransferBufferLocation transfer_location = {.transfer_buffer = vertex_transfer_buffer, .offset = 0};
    SDL_GPUBufferRegion buffer_region = {.buffer = vertex_buffer, .offset = 0, .size = sizeof(quad_vertices)};
    SDL_UploadToGPUBuffer(copy_pass, &transfer_location, &buffer_region, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
  }

  void object_graphics::bind_pipeline_and_buffers(SDL_GPURenderPass *render_pass)
  {
    SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
    SDL_GPUBufferBinding vertex_buffer_binding = {.buffer = vertex_buffer, .offset = 0};
    SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);
    SDL_GPUBufferBinding index_buffer_binding = {.buffer = index_buffer, .offset = 0};
    SDL_BindGPUIndexBuffer(render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_GPUTextureSamplerBinding texture_sampler_binding = {.texture = texture_buffer, .sampler = sampler_buffer};
    SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);
  }

  void object_graphics::push_uniform_data(SDL_GPUCommandBuffer *command_buffer, const glm::mat4 &projection_matrix,
                                          const glm::mat4 &view_matrix, const glm::mat4 &model_matrix)
  {
    std::array<glm::mat4, 3> matrices = {projection_matrix, view_matrix, model_matrix};
    SDL_PushGPUVertexUniformData(command_buffer, 0, &matrices, sizeof(matrices));
  }

  void object_graphics::draw_primitives(SDL_GPURenderPass *render_pass)
  {
    SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
  }

  void object_graphics::cleanup_object(SDL_GPUDevice *gpu)
  {
    SDL_ReleaseGPUTransferBuffer(gpu, texture_transfer_buffer);
    SDL_ReleaseGPUTransferBuffer(gpu, vertex_transfer_buffer);
    SDL_ReleaseGPUSampler(gpu, sampler_buffer);
    SDL_ReleaseGPUTexture(gpu, texture_buffer);
    SDL_ReleaseGPUBuffer(gpu, index_buffer);
    SDL_ReleaseGPUBuffer(gpu, vertex_buffer);
    SDL_ReleaseGPUGraphicsPipeline(gpu, pipeline);
  }
}
