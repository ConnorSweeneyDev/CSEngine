#include "graphics.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "glm/ext/vector_uint4_sized.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"

#include "camera.hpp"
#include "declaration.hpp"
#include "exception.hpp"
#include "name.hpp"
#include "object.hpp"
#include "resource.hpp"
#include "system.hpp"
#include "utility.hpp"

namespace cse::help
{
  game_graphics::game_graphics(const double frame_rate_, const double aspect_ratio_)
    : previous{frame_rate_, aspect_ratio_}, active{frame_rate_, aspect_ratio_}
  {
  }

  void game_graphics::initialize_app()
  {
    SDL_SetLogPriorities(debug ? SDL_LOG_PRIORITY_DEBUG : SDL_LOG_PRIORITY_ERROR);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game"))
      throw sdl_exception("Could not set app metadata type");
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING, "Connor.Sweeney.Engine"))
      throw sdl_exception("Could not set app metadata identifier");
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "CSEngine"))
      throw sdl_exception("Could not set app metadata name");
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, "0.0.0"))
      throw sdl_exception("Could not set app metadata version");
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "Connor Sweeney"))
      throw sdl_exception("Could not set app metadata creator");
    if (!SDL_Init(SDL_INIT_VIDEO)) throw sdl_exception("SDL could not be initialized");
  }

  void game_graphics::cleanup_app() { SDL_Quit(); }

  void game_graphics::update_previous()
  {
    previous.aspect_ratio = active.aspect_ratio;
    previous.frame_rate = active.frame_rate;
  }

  window_graphics::window_graphics(const std::string &title_) : previous{title_}, active{title_}
  {
    active.title.change = [this]() { handle_title_change(); };
  }

  window_graphics::~window_graphics()
  {
    render_pass = nullptr;
    depth_texture = nullptr;
    swapchain_texture = nullptr;
    command_buffer = nullptr;
    gpu = nullptr;
    instance = nullptr;
  }

  void window_graphics::create_window(const unsigned int width, const unsigned int height, int &left, int &top,
                                      SDL_DisplayID &display_index, const bool fullscreen, const bool vsync)
  {
    instance = SDL_CreateWindow(active.title->c_str(), static_cast<int>(width), static_cast<int>(height),
                                SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
    if (!instance) throw sdl_exception("Could not create window");
    windowed_width = width;
    windowed_height = height;
    display_index = SDL_GetPrimaryDisplay();
    if (display_index == 0) throw sdl_exception("Could not get primary display");
    auto position{calculate_display_center(display_index, width, height)};
    left = static_cast<int>(position.x);
    top = static_cast<int>(position.y);
    windowed_left = left;
    windowed_top = top;
    if (!SDL_SetWindowPosition(instance, left, top))
      throw sdl_exception("Could not set window position to {}, {}", left, top);
    gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, debug, "vulkan");
    if (!gpu) throw sdl_exception("Could not create GPU device");
    if (!SDL_ClaimWindowForGPUDevice(gpu, instance)) throw sdl_exception("Could not claim window for GPU device");
    if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
      throw sdl_exception("Could not enable VSYNC");
    if (fullscreen) handle_fullscreen(fullscreen, display_index);
    if (!vsync) handle_vsync(vsync);
    if (!depth_texture) generate_depth_texture(width, height);
    SDL_ShowWindow(instance);
  }

  bool window_graphics::acquire_swapchain_texture()
  {
    command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    if (!command_buffer) throw sdl_exception("Could not acquire GPU command buffer");
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, instance, &swapchain_texture, nullptr, nullptr))
      throw sdl_exception("Could not acquire GPU swapchain texture");
    if (!swapchain_texture)
    {
      if (!SDL_SubmitGPUCommandBuffer(command_buffer)) throw sdl_exception("Could not submit GPU command buffer");
      return false;
    }
    return true;
  }

  void window_graphics::start_render_pass(const unsigned int width, const unsigned int height, const float aspect_ratio)
  {
    SDL_GPUColorTargetInfo color_target_info{};
    color_target_info.texture = swapchain_texture;
    color_target_info.clear_color = {0.1f, 0.1f, 0.1f, 1.0f};
    color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPUDepthStencilTargetInfo depth_stencil_target_info{};
    depth_stencil_target_info.texture = depth_texture;
    depth_stencil_target_info.clear_depth = 1.0f;
    depth_stencil_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_stencil_target_info.store_op = SDL_GPU_STOREOP_STORE;
    render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, &depth_stencil_target_info);
    if (!render_pass) throw sdl_exception("Could not begin GPU render pass");
    float viewport_x{}, viewport_y{}, viewport_width{}, viewport_height{};
    if ((static_cast<float>(width) / static_cast<float>(height)) > aspect_ratio)
    {
      viewport_height = static_cast<float>(height);
      viewport_width = viewport_height * aspect_ratio;
      viewport_y = 0.0f;
      viewport_x = (static_cast<float>(width) - viewport_width) / 2.0f;
    }
    else
    {
      viewport_width = static_cast<float>(width);
      viewport_height = viewport_width / aspect_ratio;
      viewport_x = 0.0f;
      viewport_y = (static_cast<float>(height) - viewport_height) / 2.0f;
    }
    SDL_GPUViewport viewport{.x = viewport_x,
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
    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) throw sdl_exception("Could not submit GPU command buffer");
  }

  void window_graphics::generate_depth_texture(const unsigned int width, const unsigned int height)
  {
    if (depth_texture)
    {
      SDL_ReleaseGPUTexture(gpu, depth_texture);
      depth_texture = nullptr;
    }
    const auto type{SDL_GPU_TEXTURETYPE_2D};
    const auto usage{SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET};
    const std::array<SDL_GPUTextureFormat, 3> potential_formats{
      SDL_GPU_TEXTUREFORMAT_D32_FLOAT, SDL_GPU_TEXTUREFORMAT_D24_UNORM, SDL_GPU_TEXTUREFORMAT_D16_UNORM};
    SDL_GPUTextureCreateInfo depth_texture_info{
      .type = type,
      .format = [this, &potential_formats]() -> SDL_GPUTextureFormat
      {
        for (const auto &potential_format : potential_formats)
          if (SDL_GPUTextureSupportsFormat(gpu, potential_format, type, usage)) return potential_format;
        return {};
      }(),
      .usage = usage,
      .width = width,
      .height = height,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = SDL_GPU_SAMPLECOUNT_1,
      .props = 0};
    if (depth_texture_info.format == SDL_GPU_TEXTUREFORMAT_INVALID)
      throw sdl_exception("No supported depth texture format found");
    depth_texture = SDL_CreateGPUTexture(gpu, &depth_texture_info);
    if (!depth_texture) throw sdl_exception("Could not create depth texture");
  }

  glm::uvec2 window_graphics::calculate_display_center(const SDL_DisplayID display_index, const unsigned int width,
                                                       const unsigned int height)
  {
    SDL_Rect display_bounds{};
    if (!SDL_GetDisplayBounds(display_index, &display_bounds))
      throw sdl_exception("Could not get bounds for display {}", display_index);
    return {display_bounds.x + (display_bounds.w - static_cast<int>(width)) / 2,
            display_bounds.y + (display_bounds.h - static_cast<int>(height)) / 2};
  }

  void window_graphics::handle_title_change()
  {
    if (!SDL_SetWindowTitle(instance, active.title->c_str())) throw sdl_exception("Could not set window title");
  }

  void window_graphics::handle_move(int &left, int &top, SDL_DisplayID &display_index, const bool fullscreen)
  {
    if (fullscreen) return;
    if (!SDL_GetWindowPosition(instance, &left, &top)) throw sdl_exception("Could not get window position");
    windowed_left = left;
    windowed_top = top;
    display_index = SDL_GetDisplayForWindow(instance);
    if (display_index == 0) throw sdl_exception("Could not get window display index");
  }

  void window_graphics::handle_manual_move(const int left, const int top, const bool fullscreen)
  {
    if (!fullscreen)
    {
      windowed_left = left;
      windowed_top = top;
    }
    if (!SDL_SetWindowPosition(instance, left, top))
      throw sdl_exception("Could not set window position to {}, {}", left, top);
  }

  void window_graphics::handle_manual_display_move(const unsigned int width, const unsigned int height, int &left,
                                                   int &top, const SDL_DisplayID display_index, const bool fullscreen)
  {
    auto position{calculate_display_center(display_index, width, height)};
    left = static_cast<int>(position.x);
    top = static_cast<int>(position.y);
    if (!fullscreen)
    {
      windowed_left = left;
      windowed_top = top;
    }
    if (!SDL_SetWindowPosition(instance, left, top))
      throw sdl_exception("Could not set window position centered on display {}", display_index);
  }

  void window_graphics::handle_resize(unsigned int &width, unsigned int &height, SDL_DisplayID &display_index,
                                      const bool fullscreen)
  {
    int current_width{}, current_height{};
    SDL_GetWindowSize(instance, &current_width, &current_height);
    if (current_width <= 0) current_width = 1;
    if (current_height <= 0) current_height = 1;
    width = static_cast<unsigned int>(current_width);
    height = static_cast<unsigned int>(current_height);
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
      throw sdl_exception("Could not set window size to {}, {}", width, height);
    generate_depth_texture(width, height);
  }

  void window_graphics::handle_fullscreen(const bool fullscreen, const SDL_DisplayID display_index)
  {
    if (fullscreen)
    {
      SDL_Rect display_bounds{};
      if (!SDL_GetDisplayBounds(display_index, &display_bounds))
        throw sdl_exception("Could not get bounds for display {}", display_index);
      if (!SDL_SetWindowBordered(instance, false)) throw sdl_exception("Could not set window borderless");
      if (!SDL_SetWindowSize(instance, display_bounds.w, display_bounds.h))
        throw sdl_exception("Could not set window size to {}, {} on display {}", display_bounds.w, display_bounds.h,
                            display_index);
      auto position{calculate_display_center(display_index, static_cast<unsigned int>(display_bounds.w),
                                             static_cast<unsigned int>(display_bounds.h))};
      if (!SDL_SetWindowPosition(instance, static_cast<int>(position.x), static_cast<int>(position.y)))
        throw sdl_exception("Could not set window position centered on display {}", display_index);
      return;
    }
    if (!SDL_SetWindowBordered(instance, true)) throw sdl_exception("Could not set window bordered");
    if (!SDL_SetWindowSize(instance, static_cast<int>(windowed_width), static_cast<int>(windowed_height)))
      throw sdl_exception("Could not set window size to {}, {}", windowed_width, windowed_height);
    if (!SDL_SetWindowPosition(instance, windowed_left, windowed_top))
      throw sdl_exception("Could not set window position to {}, {}", windowed_left, windowed_top);
  }

  void window_graphics::handle_vsync(const bool vsync)
  {
    if (vsync)
    {
      if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
        throw sdl_exception("Could not enable VSYNC");
      return;
    }
    if (SDL_WindowSupportsGPUPresentMode(gpu, instance, SDL_GPU_PRESENTMODE_IMMEDIATE))
      if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                         SDL_GPU_PRESENTMODE_IMMEDIATE))
        throw sdl_exception("Could not disable VSYNC");
  }

  void window_graphics::destroy_window()
  {
    SDL_ReleaseGPUTexture(gpu, depth_texture);
    SDL_ReleaseWindowFromGPUDevice(gpu, instance);
    SDL_DestroyGPUDevice(gpu);
    SDL_DestroyWindow(instance);
  }

  void window_graphics::update_previous() { previous.title = active.title; }

  std::vector<std::shared_ptr<object>>
  scene_graphics::generate_render_order(const std::shared_ptr<camera> camera,
                                        const std::unordered_map<help::name, std::shared_ptr<object>> &objects,
                                        const std::unordered_set<help::name> &removals, const double alpha)
  {
    std::vector<std::shared_ptr<object>> render_order{};
    render_order.reserve(objects.size() - removals.size());
    for (const auto &[name, object] : objects)
    {
      if (removals.contains(name)) continue;
      render_order.emplace_back(object);
    }
    auto camera_translation = camera->state.previous.translation.value +
                              (camera->state.active.translation.value - camera->state.previous.translation.value) *
                                glm::vec3(static_cast<float>(alpha));
    auto camera_forward = glm::normalize(camera->state.previous.forward.value +
                                         (camera->state.active.forward.value - camera->state.previous.forward.value) *
                                           glm::vec3(static_cast<float>(alpha)));
    std::sort(render_order.begin(), render_order.end(),
              [alpha, &camera_translation, &camera_forward](const auto &left, const auto &right)
              {
                float left_depth =
                  glm::dot((left->state.previous.translation.value +
                            (left->state.active.translation.value - left->state.previous.translation.value) *
                              glm::vec3(static_cast<float>(alpha))) -
                             camera_translation,
                           camera_forward);
                float right_depth =
                  glm::dot((right->state.previous.translation.value +
                            (right->state.active.translation.value - right->state.previous.translation.value) *
                              glm::vec3(static_cast<float>(alpha))) -
                             camera_translation,
                           camera_forward);
                if (!equal(left_depth, right_depth, 1e-4f)) return left_depth > right_depth;
                return left->graphics.active.property.priority < right->graphics.active.property.priority;
              });
    return render_order;
  }

  camera_graphics::camera_graphics(const double fov_) : previous{fov_}, active{fov_}, near_clip{0.01f}, far_clip{100.0f}
  {
  }

  glm::mat4 camera_graphics::calculate_projection_matrix(const double alpha, const float aspect_ratio)
  {
    return glm::perspective(glm::radians(static_cast<float>(previous.fov + (active.fov - previous.fov) * alpha)),
                            aspect_ratio, near_clip, far_clip);
  }

  void camera_graphics::update_previous() { previous.fov = active.fov; }

  object_graphics::previous::previous(const struct shader &shader_, const struct texture &texture_,
                                      const struct property &property_)
    : shader{shader_}, texture{texture_}, property{property_}
  {
    shader.vertex.change = nullptr;
    shader.fragment.change = nullptr;
    texture.image.change = nullptr;
  }

  object_graphics::object_graphics(const struct shader &shader_, const struct texture &texture_,
                                   const struct property &property_)
    : previous{shader_, texture_, property_}, active{shader_, texture_, property_}
  {
    active.shader.vertex.change = [this]() { generate_pipeline(); };
    active.shader.fragment.change = [this]() { generate_pipeline(); };
    active.texture.image.change = [this]() { generate_and_upload_texture(); };
  }

  object_graphics::~object_graphics()
  {
    active.texture.image.change = nullptr;
    active.shader.fragment.change = nullptr;
    active.shader.vertex.change = nullptr;
  }

  void object_graphics::create_pipeline_and_buffers(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    cached_instance = instance;
    cached_gpu = gpu;
    generate_pipeline();
    SDL_GPUBufferCreateInfo vertex_buffer_info{
      .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = sizeof(quad_vertices), .props = 0};
    vertex_buffer = SDL_CreateGPUBuffer(gpu, &vertex_buffer_info);
    if (!vertex_buffer) throw sdl_exception("Could not create vertex buffer for object");
    SDL_GPUBufferCreateInfo index_buffer_info{
      .usage = SDL_GPU_BUFFERUSAGE_INDEX, .size = sizeof(quad_indices), .props = 0};
    index_buffer = SDL_CreateGPUBuffer(gpu, &index_buffer_info);
    if (!index_buffer) throw sdl_exception("Could not create index buffer for object");
    SDL_GPUSamplerCreateInfo sampler_info{};
    sampler_info.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_buffer = SDL_CreateGPUSampler(gpu, &sampler_info);
    if (!sampler_buffer) throw sdl_exception("Could not create sampler for object");
    SDL_GPUTransferBufferCreateInfo vertex_transfer_buffer_info{
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = sizeof(quad_vertices) + sizeof(quad_indices), .props = 0};
    vertex_transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &vertex_transfer_buffer_info);
    if (!vertex_transfer_buffer) throw sdl_exception("Could not create transfer buffer for buffer object");
    auto start{static_cast<char *>(SDL_MapGPUTransferBuffer(gpu, vertex_transfer_buffer, false))};
    auto vertex{reinterpret_cast<struct vertex_data *>(start)};
    if (!vertex) throw sdl_exception("Could not map vertex data for object");
    quad_vertices = std::array<struct vertex_data, 4>{{{1.0f, 1.0f, 0.0f, 0, 0, 0, 0, 1.0f, 1.0f},
                                                       {1.0f, -1.0f, 0.0f, 0, 0, 0, 0, 1.0f, 0.0f},
                                                       {-1.0f, 1.0f, 0.0f, 0, 0, 0, 0, 0.0f, 1.0f},
                                                       {-1.0f, -1.0f, 0.0f, 0, 0, 0, 0, 0.0f, 0.0f}}};
    std::copy(quad_vertices.begin(), quad_vertices.end(), vertex);
    auto index{reinterpret_cast<Uint16 *>(start + sizeof(quad_vertices))};
    if (!index) throw sdl_exception("Could not map index data for object");
    quad_indices = std::array<Uint16, 6>({3, 1, 0, 3, 0, 2});
    std::copy(quad_indices.begin(), quad_indices.end(), index);
    SDL_UnmapGPUTransferBuffer(gpu, vertex_transfer_buffer);
    generate_and_upload_texture();
  }

  void object_graphics::upload_static_buffers(SDL_GPUDevice *gpu)
  {
    auto *command_buffer{SDL_AcquireGPUCommandBuffer(gpu)};
    if (!command_buffer) throw sdl_exception("Could not acquire GPU command buffer for object");
    auto *copy_pass{SDL_BeginGPUCopyPass(command_buffer)};
    if (!copy_pass) throw sdl_exception("Could not begin GPU copy pass for object");
    SDL_GPUTransferBufferLocation index_transfer_buffer_location{.transfer_buffer = vertex_transfer_buffer,
                                                                 .offset = sizeof(quad_vertices)};
    SDL_GPUBufferRegion index_buffer_region{.buffer = index_buffer, .offset = 0, .size = sizeof(quad_indices)};
    SDL_UploadToGPUBuffer(copy_pass, &index_transfer_buffer_location, &index_buffer_region, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
  }

  void object_graphics::upload_dynamic_buffers(SDL_GPUDevice *gpu, const double alpha)
  {
    auto start{static_cast<char *>(SDL_MapGPUTransferBuffer(gpu, vertex_transfer_buffer, false))};
    auto vertex{reinterpret_cast<struct vertex_data *>(start)};
    if (!vertex) throw sdl_exception("Could not map vertex data for object");
    auto &frame{active.texture.animation.frame};
    auto size{active.texture.group.frames.size()};
    if (frame >= size) frame = size - 1;
    const auto &frame_coords{active.texture.group.frames[frame].coords};
    const auto color{
      glm::u8vec4(glm::vec4(previous.texture.color) +
                  (glm::vec4(active.texture.color) - glm::vec4(previous.texture.color)) * static_cast<float>(alpha))};
    const auto left{active.texture.flip.horizontal ? frame_coords.right : frame_coords.left},
      right{active.texture.flip.horizontal ? frame_coords.left : frame_coords.right},
      top{active.texture.flip.vertical ? frame_coords.bottom : frame_coords.top},
      bottom{active.texture.flip.vertical ? frame_coords.top : frame_coords.bottom};
    quad_vertices =
      std::array<struct vertex_data, 4>{{{1.0f, 1.0f, 0.0f, color.r, color.g, color.b, color.a, right, top},
                                         {1.0f, -1.0f, 0.0f, color.r, color.g, color.b, color.a, right, bottom},
                                         {-1.0f, 1.0f, 0.0f, color.r, color.g, color.b, color.a, left, top},
                                         {-1.0f, -1.0f, 0.0f, color.r, color.g, color.b, color.a, left, bottom}}};
    std::copy(quad_vertices.begin(), quad_vertices.end(), vertex);
    SDL_UnmapGPUTransferBuffer(gpu, vertex_transfer_buffer);
    auto *command_buffer{SDL_AcquireGPUCommandBuffer(gpu)};
    if (!command_buffer) throw sdl_exception("Could not acquire GPU command buffer for object");
    auto *copy_pass{SDL_BeginGPUCopyPass(command_buffer)};
    if (!copy_pass) throw sdl_exception("Could not begin GPU copy pass for object");
    SDL_GPUTransferBufferLocation transfer_location{.transfer_buffer = vertex_transfer_buffer, .offset = 0};
    SDL_GPUBufferRegion buffer_region{.buffer = vertex_buffer, .offset = 0, .size = sizeof(quad_vertices)};
    SDL_UploadToGPUBuffer(copy_pass, &transfer_location, &buffer_region, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
  }

  void object_graphics::generate_pipeline()
  {
    if (!cached_gpu) return;
    if (pipelines.opaque)
    {
      SDL_ReleaseGPUGraphicsPipeline(cached_gpu, pipelines.opaque);
      pipelines.opaque = nullptr;
    }
    if (pipelines.transparent)
    {
      SDL_ReleaseGPUGraphicsPipeline(cached_gpu, pipelines.transparent);
      pipelines.transparent = nullptr;
    }
    const auto backend_formats{SDL_GetGPUShaderFormats(cached_gpu)};
    if (!(backend_formats & SDL_GPU_SHADERFORMAT_SPIRV))
      throw sdl_exception("No supported vulkan shader formats for object");
    SDL_GPUShaderCreateInfo vertex_shader_info{.code_size = active.shader.vertex->source.size(),
                                               .code = active.shader.vertex->source.data(),
                                               .entrypoint = "main",
                                               .format = SDL_GPU_SHADERFORMAT_SPIRV,
                                               .stage = SDL_GPU_SHADERSTAGE_VERTEX,
                                               .num_samplers = 0,
                                               .num_storage_textures = 0,
                                               .num_storage_buffers = 0,
                                               .num_uniform_buffers = 1,
                                               .props = 0};
    auto *vertex_shader{SDL_CreateGPUShader(cached_gpu, &vertex_shader_info)};
    if (!vertex_shader) throw sdl_exception("Could not create vertex shader for object");
    SDL_GPUShaderCreateInfo fragment_shader_info{.code_size = active.shader.fragment->source.size(),
                                                 .code = active.shader.fragment->source.data(),
                                                 .entrypoint = "main",
                                                 .format = SDL_GPU_SHADERFORMAT_SPIRV,
                                                 .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
                                                 .num_samplers = 1,
                                                 .num_storage_textures = 0,
                                                 .num_storage_buffers = 0,
                                                 .num_uniform_buffers = 1,
                                                 .props = 0};
    auto *fragment_shader{SDL_CreateGPUShader(cached_gpu, &fragment_shader_info)};
    if (!fragment_shader) throw sdl_exception("Could not create fragment shader for object");
    SDL_GPUVertexBufferDescription vertex_buffer_description{
      .slot = 0, .pitch = sizeof(vertex_data), .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX, .instance_step_rate = 0};
    std::array<SDL_GPUVertexAttribute, 3> vertex_attributes{
      {{0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0},
       {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
        sizeof(vertex_data::x) + sizeof(vertex_data::y) + sizeof(vertex_data::z)},
       {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
        sizeof(vertex_data::x) + sizeof(vertex_data::y) + sizeof(vertex_data::z) + sizeof(vertex_data::r) +
          sizeof(vertex_data::g) + sizeof(vertex_data::b) + sizeof(vertex_data::a)}}};
    SDL_GPUVertexInputState vertex_input_state{.vertex_buffer_descriptions = &vertex_buffer_description,
                                               .num_vertex_buffers = 1,
                                               .vertex_attributes = vertex_attributes.data(),
                                               .num_vertex_attributes = 3};
    SDL_GPURasterizerState rasterizer_state{};
    rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    SDL_GPUColorTargetDescription opaque_color_target_description{};
    opaque_color_target_description.format = SDL_GetGPUSwapchainTextureFormat(cached_gpu, cached_instance);
    SDL_GPUDepthStencilState opaque_depth_stencil_state{};
    opaque_depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    opaque_depth_stencil_state.enable_depth_test = true;
    opaque_depth_stencil_state.enable_depth_write = true;
    const auto type{SDL_GPU_TEXTURETYPE_2D};
    const std::array<SDL_GPUTextureFormat, 3> potential_formats{
      SDL_GPU_TEXTUREFORMAT_D32_FLOAT, SDL_GPU_TEXTUREFORMAT_D24_UNORM, SDL_GPU_TEXTUREFORMAT_D16_UNORM};
    SDL_GPUGraphicsPipelineTargetInfo opaque_target_info{};
    opaque_target_info.color_target_descriptions = &opaque_color_target_description;
    opaque_target_info.num_color_targets = 1;
    opaque_target_info.depth_stencil_format = [this, &potential_formats]() -> SDL_GPUTextureFormat
    {
      for (const auto &potential_format : potential_formats)
        if (SDL_GPUTextureSupportsFormat(cached_gpu, potential_format, type, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
          return potential_format;
      return {};
    }();
    opaque_target_info.has_depth_stencil_target = true;
    if (opaque_target_info.depth_stencil_format == SDL_GPU_TEXTUREFORMAT_INVALID)
      throw sdl_exception("No supported depth stencil format found for object");
    SDL_GPUGraphicsPipelineCreateInfo opaque_pipeline_info{};
    opaque_pipeline_info.vertex_shader = vertex_shader;
    opaque_pipeline_info.fragment_shader = fragment_shader;
    opaque_pipeline_info.vertex_input_state = vertex_input_state;
    opaque_pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    opaque_pipeline_info.rasterizer_state = rasterizer_state;
    opaque_pipeline_info.depth_stencil_state = opaque_depth_stencil_state;
    opaque_pipeline_info.target_info = opaque_target_info;
    pipelines.opaque = SDL_CreateGPUGraphicsPipeline(cached_gpu, &opaque_pipeline_info);
    if (!pipelines.opaque) throw sdl_exception("Could not create graphics pipeline for object");
    SDL_GPUColorTargetBlendState blend_state{};
    blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    blend_state.color_write_mask = 0;
    blend_state.enable_blend = true;
    blend_state.enable_color_write_mask = false;
    SDL_GPUColorTargetDescription transparent_color_target_description{};
    transparent_color_target_description.format = SDL_GetGPUSwapchainTextureFormat(cached_gpu, cached_instance);
    transparent_color_target_description.blend_state = blend_state;
    SDL_GPUDepthStencilState transparent_depth_stencil_state{};
    transparent_depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    transparent_depth_stencil_state.enable_depth_test = true;
    transparent_depth_stencil_state.enable_depth_write = false;
    SDL_GPUGraphicsPipelineTargetInfo transparent_target_info{};
    transparent_target_info.color_target_descriptions = &transparent_color_target_description;
    transparent_target_info.num_color_targets = 1;
    transparent_target_info.depth_stencil_format = opaque_target_info.depth_stencil_format;
    transparent_target_info.has_depth_stencil_target = true;
    SDL_GPUGraphicsPipelineCreateInfo transparent_pipeline_info{};
    transparent_pipeline_info.vertex_shader = vertex_shader;
    transparent_pipeline_info.fragment_shader = fragment_shader;
    transparent_pipeline_info.vertex_input_state = vertex_input_state;
    transparent_pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    transparent_pipeline_info.rasterizer_state = rasterizer_state;
    transparent_pipeline_info.depth_stencil_state = transparent_depth_stencil_state;
    transparent_pipeline_info.target_info = transparent_target_info;
    pipelines.transparent = SDL_CreateGPUGraphicsPipeline(cached_gpu, &transparent_pipeline_info);
    if (!pipelines.transparent) throw sdl_exception("Could not create transparent graphics pipeline for object");
    SDL_ReleaseGPUShader(cached_gpu, fragment_shader);
    SDL_ReleaseGPUShader(cached_gpu, vertex_shader);
  }

  void object_graphics::generate_and_upload_texture()
  {
    if (!cached_gpu) return;
    if (texture_transfer_buffer)
    {
      SDL_ReleaseGPUTransferBuffer(cached_gpu, texture_transfer_buffer);
      texture_transfer_buffer = nullptr;
    }
    if (texture_buffer)
    {
      SDL_ReleaseGPUTexture(cached_gpu, texture_buffer);
      texture_buffer = nullptr;
    }
    const auto type{SDL_GPU_TEXTURETYPE_2D};
    const auto format{SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM};
    const auto usage{SDL_GPU_TEXTUREUSAGE_SAMPLER};
    if (!SDL_GPUTextureSupportsFormat(cached_gpu, format, type, usage))
      throw sdl_exception("No supported texture format found for object");
    SDL_GPUTextureCreateInfo texture_info{.type = type,
                                          .format = format,
                                          .usage = usage,
                                          .width = active.texture.image->width,
                                          .height = active.texture.image->height,
                                          .layer_count_or_depth = 1,
                                          .num_levels = 1,
                                          .sample_count = SDL_GPU_SAMPLECOUNT_1,
                                          .props = 0};
    texture_buffer = SDL_CreateGPUTexture(cached_gpu, &texture_info);
    if (!texture_buffer) throw sdl_exception("Could not create texture for object");
    SDL_GPUTransferBufferCreateInfo texture_transfer_buffer_info{
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = active.texture.image->width * active.texture.image->height * active.texture.image->channels,
      .props = 0};
    texture_transfer_buffer = SDL_CreateGPUTransferBuffer(cached_gpu, &texture_transfer_buffer_info);
    if (!texture_transfer_buffer) throw sdl_exception("Could not create transfer buffer for texture for object");
    auto start{static_cast<char *>(SDL_MapGPUTransferBuffer(cached_gpu, texture_transfer_buffer, false))};
    auto texture_data{reinterpret_cast<Uint8 *>(start)};
    if (!texture_data) throw sdl_exception("Could not map texture data for object");
    SDL_memcpy(texture_data, active.texture.image->data.data(),
               active.texture.image->width * active.texture.image->height * active.texture.image->channels);
    SDL_UnmapGPUTransferBuffer(cached_gpu, texture_transfer_buffer);
    auto *command_buffer{SDL_AcquireGPUCommandBuffer(cached_gpu)};
    if (!command_buffer) throw sdl_exception("Could not acquire GPU command buffer for object");
    auto *copy_pass{SDL_BeginGPUCopyPass(command_buffer)};
    if (!copy_pass) throw sdl_exception("Could not begin GPU copy pass for object");
    SDL_GPUTextureTransferInfo texture_transfer_info{
      .transfer_buffer = texture_transfer_buffer, .offset = 0, .pixels_per_row = 0, .rows_per_layer = 0};
    SDL_GPUTextureRegion texture_region{.texture = texture_buffer,
                                        .mip_level = 0,
                                        .layer = 0,
                                        .x = 0,
                                        .y = 0,
                                        .z = 0,
                                        .w = active.texture.image->width,
                                        .h = active.texture.image->height,
                                        .d = 1};
    SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    SDL_ReleaseGPUTransferBuffer(cached_gpu, texture_transfer_buffer);
    texture_transfer_buffer = nullptr;
  }

  void object_graphics::update_animation(const float poll_rate)
  {
    auto &group{active.texture.group};
    auto &animation{active.texture.animation};
    auto no_frames{group.frames.empty()};
    auto frame_count{group.frames.size()};
    if (no_frames)
      animation.frame = 0;
    else if (animation.frame >= frame_count)
      animation.frame = frame_count - 1;
    if (animation.speed > 0.0 && !no_frames)
    {
      animation.elapsed += static_cast<double>(poll_rate) * animation.speed;
      while (true)
      {
        auto duration = group.frames[animation.frame].duration;
        if (duration > 0 && animation.elapsed < duration) break;
        if (animation.frame < frame_count - 1)
        {
          if (duration > 0) animation.elapsed -= duration;
          animation.frame++;
        }
        else if (animation.loop)
        {
          if (duration > 0)
            animation.elapsed -= duration;
          else
            break;
          animation.frame = 0;
        }
        else
          break;
      }
    }
    else if (animation.speed < 0.0 && !no_frames)
    {
      animation.elapsed += static_cast<double>(poll_rate) * animation.speed;
      while (animation.elapsed < 0)
        if (animation.frame > 0)
        {
          animation.frame--;
          auto duration = group.frames[animation.frame].duration;
          if (duration > 0) animation.elapsed += duration;
        }
        else if (animation.loop)
        {
          if (group.frames[0].duration <= 0) break;
          animation.frame = frame_count - 1;
          auto duration = group.frames[animation.frame].duration;
          if (duration > 0) animation.elapsed += duration;
        }
        else
          break;
    }
  }

  void object_graphics::bind_pipeline_and_buffers(SDL_GPURenderPass *render_pass, const double alpha)
  {
    SDL_BindGPUGraphicsPipeline(
      render_pass,
      (previous.texture.transparency + (active.texture.transparency - previous.texture.transparency) * alpha) < 1.0
        ? pipelines.transparent
        : pipelines.opaque);
    SDL_GPUBufferBinding vertex_buffer_binding{.buffer = vertex_buffer, .offset = 0};
    SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);
    SDL_GPUBufferBinding index_buffer_binding{.buffer = index_buffer, .offset = 0};
    SDL_BindGPUIndexBuffer(render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_GPUTextureSamplerBinding texture_sampler_binding{.texture = texture_buffer, .sampler = sampler_buffer};
    SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);
  }

  void object_graphics::push_uniform_data(SDL_GPUCommandBuffer *command_buffer,
                                          const std::array<glm::mat4, 3> &matrices, const double alpha)
  {
    SDL_PushGPUVertexUniformData(command_buffer, 0, &matrices, sizeof(matrices));
    const auto transparency{static_cast<float>(previous.texture.transparency +
                                               (active.texture.transparency - previous.texture.transparency) * alpha)};
    SDL_PushGPUFragmentUniformData(command_buffer, 0, &transparency, sizeof(transparency));
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
    SDL_ReleaseGPUGraphicsPipeline(gpu, pipelines.transparent);
    SDL_ReleaseGPUGraphicsPipeline(gpu, pipelines.opaque);
    texture_transfer_buffer = nullptr;
    vertex_transfer_buffer = nullptr;
    sampler_buffer = nullptr;
    texture_buffer = nullptr;
    index_buffer = nullptr;
    vertex_buffer = nullptr;
    pipelines.transparent = nullptr;
    pipelines.opaque = nullptr;
    cached_gpu = nullptr;
    cached_instance = nullptr;
  }

  void object_graphics::update_previous()
  {
    previous.shader.vertex = active.shader.vertex;
    previous.shader.fragment = active.shader.fragment;
    previous.texture.image = active.texture.image;
    previous.texture.group = active.texture.group;
    previous.texture.animation = active.texture.animation;
    previous.texture.flip = active.texture.flip;
    previous.texture.color = active.texture.color;
    previous.texture.transparency = active.texture.transparency;
    previous.property.priority = active.property.priority;
  }
}
