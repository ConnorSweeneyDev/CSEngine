#include "graphics.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"

#include "exception.hpp"
#include "resource.hpp"

namespace cse::helper
{
  window_graphics::window_graphics(const std::string &title_, const unsigned int starting_width_,
                                   const unsigned int starting_height_, const bool fullscreen_, const bool vsync_)
    : fullscreen(fullscreen_), vsync(vsync_), title(title_), starting_width(starting_width_),
      starting_height(starting_height_), width(starting_width_), height(starting_height_)
  {
    fullscreen.on_change = [this]()
    {
      if (fullscreen)
        enable_fullscreen();
      else
        disable_fullscreen();
    };

    vsync.on_change = [this]()
    {
      if (vsync)
        enable_vsync();
      else
        disable_vsync();
    };
  }

  window_graphics::~window_graphics()
  {
    fullscreen.on_change = nullptr;
    vsync.on_change = nullptr;
  }

  void window_graphics::initialize_app()
  {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game"))
      throw cse::utility::sdl_exception("Could not set app metadata type for window '{}'", title);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING, "Connor.Sweeney.Engine"))
      throw cse::utility::sdl_exception("Could not set app metadata identifier for window '{}'", title);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "CSEngine"))
      throw cse::utility::sdl_exception("Could not set app metadata name for window '{}'", title);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, "0.0.0"))
      throw cse::utility::sdl_exception("Could not set app metadata version for window '{}'", title);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "Connor Sweeney"))
      throw cse::utility::sdl_exception("Could not set app metadata creator for window '{}'", title);
    if (!SDL_Init(SDL_INIT_VIDEO))
      throw cse::utility::sdl_exception("SDL could not be initialized for window '{}'", title);
  }

  void window_graphics::create_window()
  {
    instance = SDL_CreateWindow(title.c_str(), static_cast<int>(starting_width), static_cast<int>(starting_height),
                                SDL_WINDOW_HIDDEN);
    if (!instance) throw cse::utility::sdl_exception("Could not create window '{}'", title);
    display_index = SDL_GetPrimaryDisplay();
    if (display_index == 0) throw cse::utility::sdl_exception("Could not get primary display for window '{}'", title);
    left = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    top = SDL_WINDOWPOS_CENTERED_DISPLAY(display_index);
    if (!SDL_SetWindowPosition(instance, left, top))
      throw cse::utility::sdl_exception("Could not set position to ({}, {}) for window '{}'", left, top, title);
    if (fullscreen) fullscreen.on_change();

    gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, false, nullptr);
    if (!gpu) throw cse::utility::sdl_exception("Could not create GPU device for window '{}'", title);
    if (!SDL_ClaimWindowForGPUDevice(gpu, instance))
      throw cse::utility::sdl_exception("Could not claim window for GPU device for window '{}'", title);
    if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
      throw cse::utility::sdl_exception("Could not enable VSYNC for window '{}'", title);
    if (!vsync) vsync.on_change();

    SDL_ShowWindow(instance);
  }

  bool window_graphics::create_command_and_swapchain()
  {
    command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    if (!command_buffer)
      throw cse::utility::sdl_exception("Could not acquire GPU command buffer for window '{}'", title);

    if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, instance, &swapchain_texture, nullptr, nullptr))
      throw cse::utility::sdl_exception("Could not acquire GPU swapchain texture for window '{}'", title);
    if (!swapchain_texture)
    {
      if (!SDL_SubmitGPUCommandBuffer(command_buffer))
        throw cse::utility::sdl_exception("Could not submit GPU command buffer for window '{}'", title);
      return false;
    }

    return true;
  }

  void window_graphics::create_render_pass()
  {
    SDL_GPUColorTargetInfo color_target_info(swapchain_texture, Uint32(), Uint32(), {0.1f, 0.1f, 0.1f, 1.0f},
                                             SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE);
    render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr);
    if (!render_pass) throw cse::utility::sdl_exception("Could not begin GPU render pass for window '{}'", title);
    SDL_GPUViewport viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    SDL_SetGPUViewport(render_pass, &viewport);
  }

  void window_graphics::end_render_and_submit_command()
  {
    SDL_EndGPURenderPass(render_pass);
    if (!SDL_SubmitGPUCommandBuffer(command_buffer))
      throw cse::utility::sdl_exception("Could not submit GPU command buffer for window '{}'", title);
  }

  void window_graphics::enable_fullscreen()
  {
    SDL_Rect display_bounds = {};
    if (!SDL_GetDisplayBounds(display_index, &display_bounds))
      throw utility::sdl_exception("Could not get bounds for display ({}) for window '{}'", display_index, title);
    if (!SDL_SetWindowBordered(instance, false))
      throw utility::sdl_exception("Could not set borderless for window '{}'", title);
    if (!SDL_SetWindowSize(instance, display_bounds.w, display_bounds.h))
      throw utility::sdl_exception("Could not set size to ({}, {}) on display ({}) for window '{}'", display_bounds.w,
                                   display_bounds.h, display_index, title);
    width = static_cast<unsigned int>(display_bounds.w);
    height = static_cast<unsigned int>(display_bounds.h);
    if (!SDL_SetWindowPosition(instance, SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
                               SDL_WINDOWPOS_CENTERED_DISPLAY(display_index)))
      throw utility::sdl_exception("Could not set position centered on display ({}) for window '{}'", display_index);
  }

  void window_graphics::disable_fullscreen()
  {
    if (!SDL_SetWindowBordered(instance, true))
      throw utility::sdl_exception("Could not set bordered for window '{}'", title);
    if (!SDL_SetWindowSize(instance, static_cast<int>(starting_width), static_cast<int>(starting_height)))
      throw utility::sdl_exception("Could not set size to ({}, {}) for window '{}'", starting_width, starting_height,
                                   title);
    width = starting_width;
    height = starting_height;
    if (!SDL_SetWindowPosition(instance, left, top))
      throw utility::sdl_exception("Could not set position to ({}, {}) for window '{}'", left, top, title);
  }

  void window_graphics::enable_vsync()
  {
    if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
      throw utility::sdl_exception("Could not enable VSYNC for window '{}'", title);
  }

  void window_graphics::disable_vsync()
  {
    if (SDL_WindowSupportsGPUPresentMode(gpu, instance, SDL_GPU_PRESENTMODE_IMMEDIATE))
      if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                         SDL_GPU_PRESENTMODE_IMMEDIATE))
        throw utility::sdl_exception("Could not disable VSYNC for window '{}'", title);
  }

  void window_graphics::handle_move()
  {
    if (fullscreen) return;
    if (!SDL_GetWindowPosition(instance, &left, &top))
      throw utility::sdl_exception("Could not get position for window '{}'", title);
    display_index = SDL_GetDisplayForWindow(instance);
    if (display_index == 0) throw utility::sdl_exception("Could not get display index for window '{}'", title);
  }

  void window_graphics::cleanup_gpu_and_app()
  {
    SDL_ReleaseWindowFromGPUDevice(gpu, instance);
    SDL_DestroyGPUDevice(gpu);
    SDL_DestroyWindow(instance);
    render_pass = nullptr;
    swapchain_texture = nullptr;
    command_buffer = nullptr;
    gpu = nullptr;
    instance = nullptr;
    SDL_Quit();
  }

  camera_graphics::camera_graphics(const float fov_) : fov(fov_), near_clip(0.01f), far_clip(100.0f) {}

  glm::mat4 camera_graphics::calculate_projection_matrix(const unsigned int width, const unsigned int height)
  {
    glm::mat4 projection_matrix =
      glm::perspective(glm::radians(fov), static_cast<float>(width) / static_cast<float>(height), near_clip, far_clip);
    return projection_matrix;
  }

  glm::mat4 camera_graphics::calculate_view_matrix(const glm::vec3 &translation, const glm::vec3 &forward,
                                                   const glm::vec3 &up, const float scale_factor)
  {
    glm::mat4 view_matrix = glm::lookAt(translation * scale_factor, (translation * scale_factor) + forward, up);
    return view_matrix;
  }

  object_graphics::object_graphics(const resource::compiled_shader &vertex_shader_,
                                   const resource::compiled_shader &fragment_shader_,
                                   const resource::compiled_texture &texture_, unsigned int current_frame_)
    : shader(vertex_shader_, fragment_shader_), texture(texture_, current_frame_)
  {
  }

  void object_graphics::create_pipeline(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    SDL_GPUShaderFormat current_format = SDL_GPU_SHADERFORMAT_INVALID;
    const SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(gpu);
    if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV)
      current_format = SDL_GPU_SHADERFORMAT_SPIRV;
    else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL)
      current_format = SDL_GPU_SHADERFORMAT_DXIL;
    else
      throw cse::utility::sdl_exception("Could not find supported shader format for object");

    SDL_GPUShaderCreateInfo vertex_shader_info(
      current_format == SDL_GPU_SHADERFORMAT_DXIL ? shader.vertex.dxil.size() : shader.vertex.spirv.size(),
      current_format == SDL_GPU_SHADERFORMAT_DXIL ? shader.vertex.dxil.data() : shader.vertex.spirv.data(), "main",
      current_format, SDL_GPU_SHADERSTAGE_VERTEX, 0, 0, 0, 1);
    SDL_GPUShader *vertex_shader = SDL_CreateGPUShader(gpu, &vertex_shader_info);
    if (!vertex_shader) throw cse::utility::sdl_exception("Could not create vertex shader for object");
    SDL_GPUShaderCreateInfo fragment_shader_info(
      current_format == SDL_GPU_SHADERFORMAT_DXIL ? shader.fragment.dxil.size() : shader.fragment.spirv.size(),
      current_format == SDL_GPU_SHADERFORMAT_DXIL ? shader.fragment.dxil.data() : shader.fragment.spirv.data(), "main",
      current_format, SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0, 0, 0);
    SDL_GPUShader *fragment_shader = SDL_CreateGPUShader(gpu, &fragment_shader_info);
    if (!fragment_shader) throw cse::utility::sdl_exception("Could not create fragment shader for object");

    SDL_GPUVertexBufferDescription vertex_buffer_description(0, sizeof(vertex), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0);
    std::array<SDL_GPUVertexAttribute, 3> vertex_attributes(
      {{0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0},
       {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, sizeof(vertex::x) + sizeof(vertex::y) + sizeof(vertex::z)},
       {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
        sizeof(vertex::x) + sizeof(vertex::y) + sizeof(vertex::z) + sizeof(vertex::r) + sizeof(vertex::g) +
          sizeof(vertex::b) + sizeof(vertex::a)}});

    SDL_GPUVertexInputState vertex_input_state(&vertex_buffer_description, 1, vertex_attributes.data(), 3);
    SDL_GPURasterizerState rasterizer_state(SDL_GPU_FILLMODE_FILL, SDL_GPU_CULLMODE_BACK,
                                            SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE);
    SDL_GPUColorTargetDescription color_target_description(SDL_GetGPUSwapchainTextureFormat(gpu, instance));
    SDL_GPUGraphicsPipelineTargetInfo target_info(&color_target_description, 1);
    SDL_GPUGraphicsPipelineCreateInfo pipeline_info(vertex_shader, fragment_shader, vertex_input_state,
                                                    SDL_GPU_PRIMITIVETYPE_TRIANGLELIST, rasterizer_state,
                                                    SDL_GPUMultisampleState(), SDL_GPUDepthStencilState(), target_info);
    pipeline = SDL_CreateGPUGraphicsPipeline(gpu, &pipeline_info);
    if (!pipeline) throw cse::utility::sdl_exception("Could not create graphics pipeline for object");

    SDL_ReleaseGPUShader(gpu, fragment_shader);
    SDL_ReleaseGPUShader(gpu, vertex_shader);
  }

  void object_graphics::create_vertex_and_index(SDL_GPUDevice *gpu)
  {
    SDL_GPUBufferCreateInfo vertex_buffer_info(SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(quad_vertices));
    vertex_buffer = SDL_CreateGPUBuffer(gpu, &vertex_buffer_info);
    if (!vertex_buffer) throw cse::utility::sdl_exception("Could not create vertex buffer for object");

    SDL_GPUBufferCreateInfo index_buffer_info(SDL_GPU_BUFFERUSAGE_INDEX, sizeof(quad_indices));
    index_buffer = SDL_CreateGPUBuffer(gpu, &index_buffer_info);
    if (!index_buffer) throw cse::utility::sdl_exception("Could not create index buffer for object");
  }

  void object_graphics::create_sampler_and_texture(SDL_GPUDevice *gpu)
  {
    SDL_GPUSamplerCreateInfo sampler_info(SDL_GPU_FILTER_NEAREST, SDL_GPU_FILTER_NEAREST,
                                          SDL_GPU_SAMPLERMIPMAPMODE_NEAREST, SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
                                          SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
                                          SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE);
    sampler_buffer = SDL_CreateGPUSampler(gpu, &sampler_info);
    if (!sampler_buffer) throw cse::utility::sdl_exception("Could not create sampler for object");

    SDL_GPUTextureCreateInfo texture_info(SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                          SDL_GPU_TEXTUREUSAGE_SAMPLER, static_cast<Uint32>(texture.raw.width),
                                          static_cast<Uint32>(texture.raw.height), 1, 1);
    texture_buffer = SDL_CreateGPUTexture(gpu, &texture_info);
    if (!texture_buffer) throw cse::utility::sdl_exception("Could not create texture for object");
  }

  void object_graphics::transfer_vertex_and_index(SDL_GPUDevice *gpu)
  {
    SDL_GPUTransferBufferCreateInfo buffer_transfer_buffer_info(SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                                                sizeof(quad_vertices) + sizeof(quad_indices));
    buffer_transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &buffer_transfer_buffer_info);
    if (!buffer_transfer_buffer)
      throw cse::utility::sdl_exception("Could not create transfer buffer for buffer object");

    auto vertex_data = reinterpret_cast<vertex *>(SDL_MapGPUTransferBuffer(gpu, buffer_transfer_buffer, false));
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

    SDL_UnmapGPUTransferBuffer(gpu, buffer_transfer_buffer);
  }

  void object_graphics::transfer_texture(SDL_GPUDevice *gpu)
  {
    SDL_GPUTransferBufferCreateInfo texture_transfer_buffer_info(SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                                                 static_cast<Uint32>(texture.raw.width) *
                                                                   static_cast<Uint32>(texture.raw.height) *
                                                                   static_cast<Uint32>(texture.raw.channels));
    texture_transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &texture_transfer_buffer_info);
    if (!texture_transfer_buffer)
      throw cse::utility::sdl_exception("Could not create transfer buffer for texture for object");

    auto *texture_data = reinterpret_cast<Uint8 *>(SDL_MapGPUTransferBuffer(gpu, texture_transfer_buffer, false));
    if (!texture_data) throw cse::utility::sdl_exception("Could not map texture data for object");
    SDL_memcpy(texture_data, texture.raw.image.data(), texture.raw.width * texture.raw.height * texture.raw.channels);

    SDL_UnmapGPUTransferBuffer(gpu, texture_transfer_buffer);
  }

  void object_graphics::upload_to_gpu(SDL_GPUDevice *gpu)
  {
    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    if (!command_buffer) throw cse::utility::sdl_exception("Could not acquire GPU command buffer for object");
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass) throw cse::utility::sdl_exception("Could not begin GPU copy pass for object");

    SDL_GPUTransferBufferLocation vertex_transfer_buffer_location(buffer_transfer_buffer, 0);
    SDL_GPUBufferRegion vertex_buffer_region(vertex_buffer, 0, sizeof(quad_vertices));
    SDL_UploadToGPUBuffer(copy_pass, &vertex_transfer_buffer_location, &vertex_buffer_region, false);
    SDL_GPUTransferBufferLocation index_transfer_buffer_location(buffer_transfer_buffer, sizeof(quad_vertices));
    SDL_GPUBufferRegion index_buffer_region(index_buffer, 0, sizeof(quad_indices));
    SDL_UploadToGPUBuffer(copy_pass, &index_transfer_buffer_location, &index_buffer_region, false);

    SDL_GPUTextureTransferInfo texture_transfer_info(texture_transfer_buffer, 0);
    SDL_GPUTextureRegion texture_region(texture_buffer, Uint32(), Uint32(), Uint32(), Uint32(), Uint32(),
                                        static_cast<Uint32>(texture.raw.width), static_cast<Uint32>(texture.raw.height),
                                        1);
    SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    SDL_ReleaseGPUTransferBuffer(gpu, texture_transfer_buffer);
    SDL_ReleaseGPUTransferBuffer(gpu, buffer_transfer_buffer);
    texture_transfer_buffer = nullptr;
    buffer_transfer_buffer = nullptr;
  }

  void object_graphics::update_vertex(SDL_GPUDevice *gpu)
  {
    SDL_GPUTransferBufferCreateInfo buffer_info(SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, sizeof(quad_vertices));
    SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &buffer_info);
    if (!transfer_buffer) throw cse::utility::sdl_exception("Could not create transfer buffer for vertex object");

    auto vertex_data = reinterpret_cast<vertex *>(SDL_MapGPUTransferBuffer(gpu, transfer_buffer, false));
    if (!vertex_data) throw cse::utility::sdl_exception("Could not map vertex data for object");
    if (texture.current_frame >= texture.raw.frame_count)
      throw cse::utility::exception("Current frame {} (index {}) exceeds total frames {}", texture.current_frame + 1,
                                    texture.current_frame, texture.raw.frame_count);
    unsigned int frames_per_row = texture.raw.width / texture.raw.frame_width;
    unsigned int frames_per_column = texture.raw.height / texture.raw.frame_height;
    unsigned int frame_x = texture.current_frame % frames_per_row;
    unsigned int frame_y = (frames_per_column - 1) - (texture.current_frame / frames_per_row);
    float left = static_cast<float>(frame_x * texture.raw.frame_width) / static_cast<float>(texture.raw.width);
    float right = static_cast<float>((frame_x + 1) * texture.raw.frame_width) / static_cast<float>(texture.raw.width);
    float top = static_cast<float>((frame_y + 1) * texture.raw.frame_height) / static_cast<float>(texture.raw.height);
    float bottom = static_cast<float>(frame_y * texture.raw.frame_height) / static_cast<float>(texture.raw.height);
    quad_vertices = std::array<vertex, 4>({{1.0f, 1.0f, 0.0f, 0, 0, 0, 0, right, top},
                                           {1.0f, -1.0f, 0.0f, 0, 0, 0, 0, right, bottom},
                                           {-1.0f, 1.0f, 0.0f, 0, 0, 0, 0, left, top},
                                           {-1.0f, -1.0f, 0.0f, 0, 0, 0, 0, left, bottom}});
    std::copy(quad_vertices.begin(), quad_vertices.end(), vertex_data);
    SDL_UnmapGPUTransferBuffer(gpu, transfer_buffer);

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    if (!command_buffer) throw cse::utility::sdl_exception("Could not acquire GPU command buffer for object");
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass) throw cse::utility::sdl_exception("Could not begin GPU copy pass for object");

    SDL_GPUTransferBufferLocation transfer_location(transfer_buffer, 0);
    SDL_GPUBufferRegion buffer_region(vertex_buffer, 0, sizeof(quad_vertices));
    SDL_UploadToGPUBuffer(copy_pass, &transfer_location, &buffer_region, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    SDL_ReleaseGPUTransferBuffer(gpu, transfer_buffer);
    transfer_buffer = nullptr;
  }

  void object_graphics::bind_pipeline_and_buffers(SDL_GPURenderPass *render_pass)
  {
    SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
    SDL_GPUBufferBinding vertex_buffer_binding(vertex_buffer, 0);
    SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);
    SDL_GPUBufferBinding index_buffer_binding(index_buffer, 0);
    SDL_BindGPUIndexBuffer(render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_GPUTextureSamplerBinding texture_sampler_binding(texture_buffer, sampler_buffer);
    SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);
  }

  glm::mat4 object_graphics::calculate_model_matrix(const glm::vec3 &translation, const glm::vec3 &rotation,
                                                    const glm::vec3 &scale, const float scale_factor)
  {
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix =
      glm::translate(model_matrix, {std::floor((translation.x * scale_factor) / scale_factor) * scale_factor,
                                    std::floor((translation.y * scale_factor) / scale_factor) * scale_factor,
                                    std::floor((translation.z * scale_factor) / scale_factor) * scale_factor});
    model_matrix = glm::rotate(model_matrix, glm::radians(std::floor((rotation.x * 90.0f) / 90.0f) * 90.0f),
                               glm::vec3(1.0f, 0.0f, 0.0f));
    model_matrix = glm::rotate(model_matrix, glm::radians(std::floor((rotation.y * 90.0f) / 90.0f) * 90.0f),
                               glm::vec3(0.0f, 1.0f, 0.0f));
    model_matrix = glm::rotate(model_matrix, glm::radians(std::floor((rotation.z * 90.0f) / 90.0f) * 90.0f),
                               glm::vec3(0.0f, 0.0f, 1.0f));
    model_matrix =
      glm::scale(model_matrix,
                 {std::floor(scale.x) * (static_cast<float>(texture.raw.frame_width) / 50.0f),
                  std::floor(scale.y) * (static_cast<float>(texture.raw.frame_height) / 50.0f), std::floor(scale.z)});
    return model_matrix;
  }

  void object_graphics::push_uniform_data(SDL_GPUCommandBuffer *command_buffer, const glm::mat4 &model_matrix,
                                          const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix)
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
    SDL_ReleaseGPUSampler(gpu, sampler_buffer);
    SDL_ReleaseGPUTexture(gpu, texture_buffer);
    SDL_ReleaseGPUBuffer(gpu, index_buffer);
    SDL_ReleaseGPUBuffer(gpu, vertex_buffer);
    SDL_ReleaseGPUGraphicsPipeline(gpu, pipeline);

    sampler_buffer = nullptr;
    texture_buffer = nullptr;
    index_buffer = nullptr;
    vertex_buffer = nullptr;
    pipeline = nullptr;
  }
}
