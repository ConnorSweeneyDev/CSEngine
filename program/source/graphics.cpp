#include "graphics.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_double4.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/ext/vector_int2.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"

#include "camera.hpp"
#include "core.hpp"
#include "exception.hpp"
#include "name.hpp"
#include "numeric.hpp"
#include "object.hpp"
#include "resource.hpp"
#include "state.hpp"
#include "system.hpp"

namespace cse::help
{
  game_graphics::game_graphics(const double frame_, const double aspect_, const glm::dvec4 &clear_)
    : previous{frame_, aspect_, clear_}, active{frame_, aspect_, clear_}
  {
  }

  void game_graphics::update_previous()
  {
    previous.aspect = active.aspect;
    previous.frame = active.frame;
    previous.clear = active.clear;
  }

  void game_graphics::create_app()
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
    if (!SDL_Init(SDL_INIT_VIDEO)) throw sdl_exception("SDL could not be prepared");
  }

  void game_graphics::destroy_app() { SDL_Quit(); }

  window_graphics::window_graphics(const std::string &title_, const bool fullscreen_, const bool vsync_)
    : previous{title_, fullscreen_, vsync_}, active{title_, fullscreen_, vsync_}
  {
    active.title.change = [this]() { handle_title_change(); };
    active.vsync.change = [this]() { handle_vsync(); };
  }

  void window_graphics::update_previous()
  {
    previous.title = active.title;
    previous.fullscreen = active.fullscreen;
    previous.vsync = active.vsync;
  }

  void window_graphics::create_window(
    SDL_DisplayID &display, int &left, int &top, const unsigned int width, const unsigned int height,
    int &windowed_left, int &windowed_top, unsigned int &windowed_width, unsigned int &windowed_height,
    const std::function<glm::ivec2(const SDL_DisplayID display, const unsigned int width, const unsigned int height)>
      &calculate_display_center,
    const std::function<glm::ivec2(const SDL_DisplayID display, const int left, const int top)> &relative_to_absolute,
    const std::function<glm::ivec2(const SDL_DisplayID display, const int left, const int top)> &absolute_to_relative,
    const SDL_DisplayID PRIMARY, const int CENTER)
  {
    instance = SDL_CreateWindow(active.title->c_str(), static_cast<int>(width), static_cast<int>(height),
                                SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
    if (!instance) throw sdl_exception("Could not create window");

    int display_count{};
    SDL_GetDisplays(&display_count);
    if (display == PRIMARY || display > static_cast<unsigned int>(display_count)) display = SDL_GetPrimaryDisplay();
    if (display == 0) throw sdl_exception("Invalid display index {}", display);

    auto absolute_center{calculate_display_center(display, width, height)};
    auto relative_center{absolute_to_relative(display, absolute_center.x, absolute_center.y)};
    left = left == CENTER ? relative_center.x : left;
    top = top == CENTER ? relative_center.y : top;
    windowed_left = left;
    windowed_top = top;
    auto absolute{relative_to_absolute(display, left, top)};
    if (!SDL_SetWindowPosition(instance, absolute.x, absolute.y))
      throw sdl_exception("Could not set window position to {}, {}", absolute.x, absolute.y);

    windowed_width = width;
    windowed_height = height;

    gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, debug, "vulkan");
    if (!gpu) throw sdl_exception("Could not create GPU device");
    if (!SDL_ClaimWindowForGPUDevice(gpu, instance)) throw sdl_exception("Could not claim window for GPU device");
    if (!SDL_SetGPUSwapchainParameters(gpu, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
      throw sdl_exception("Could not enable VSYNC");

    if (active.fullscreen)
      handle_fullscreen(display, windowed_left, windowed_top, windowed_width, windowed_height, calculate_display_center,
                        relative_to_absolute);
    if (!active.vsync) handle_vsync();
    if (!depth_texture) generate_depth_texture(width, height, gpu, depth_texture);
    SDL_ShowWindow(instance);
  }

  void window_graphics::generate_depth_texture(const unsigned int width, const unsigned int height, SDL_GPUDevice *gpu,
                                               SDL_GPUTexture *&depth_texture)
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
      .format = [&gpu, &potential_formats]() -> SDL_GPUTextureFormat
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

  void window_graphics::start_render_pass(const unsigned int width, const unsigned int height,
                                          const glm::dvec4 &previous_clear, const glm::dvec4 &active_clear,
                                          const double previous_aspect, const double active_aspect, const double alpha)
  {
    SDL_GPUColorTargetInfo color_target_info{};
    color_target_info.texture = swapchain_texture;
    auto target_clear{previous_clear + (active_clear - previous_clear) * alpha};
    color_target_info.clear_color = {static_cast<float>(target_clear.r), static_cast<float>(target_clear.g),
                                     static_cast<float>(target_clear.b), static_cast<float>(target_clear.a)};
    color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPUDepthStencilTargetInfo depth_stencil_target_info{};
    depth_stencil_target_info.texture = depth_texture;
    depth_stencil_target_info.clear_depth = 1.0f;
    depth_stencil_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_stencil_target_info.store_op = SDL_GPU_STOREOP_STORE;
    render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, &depth_stencil_target_info);
    if (!render_pass) throw sdl_exception("Could not begin GPU render pass");
    float viewport_left{}, viewport_top{}, viewport_width{}, viewport_height{};
    auto target_aspect{static_cast<float>(previous_aspect + (active_aspect - previous_aspect) * alpha)};
    if ((static_cast<float>(width) / static_cast<float>(height)) > target_aspect)
    {
      viewport_height = static_cast<float>(height);
      viewport_width = viewport_height * target_aspect;
      viewport_top = 0.0f;
      viewport_left = (static_cast<float>(width) - viewport_width) / 2.0f;
    }
    else
    {
      viewport_width = static_cast<float>(width);
      viewport_height = viewport_width / target_aspect;
      viewport_left = 0.0f;
      viewport_top = (static_cast<float>(height) - viewport_height) / 2.0f;
    }
    SDL_GPUViewport viewport{.x = viewport_left,
                             .y = viewport_top,
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

  void window_graphics::handle_title_change()
  {
    if (!SDL_SetWindowTitle(instance, active.title->c_str())) throw sdl_exception("Could not set window title");
  }

  void window_graphics::handle_fullscreen(
    const SDL_DisplayID display, const int windowed_left, const int windowed_top, const unsigned int windowed_width,
    const unsigned int windowed_height,
    const std::function<glm::ivec2(const SDL_DisplayID display, const unsigned int width, const unsigned int height)>
      &calculate_display_center,
    const std::function<glm::ivec2(const SDL_DisplayID display, const int left, const int top)> &relative_to_absolute)
  {
    if (active.fullscreen)
    {
      SDL_Rect display_bounds{};
      if (!SDL_GetDisplayBounds(display, &display_bounds))
        throw sdl_exception("Could not get bounds for display {}", display);
      if (!SDL_SetWindowBordered(instance, false)) throw sdl_exception("Could not set window borderless");
      if (!SDL_SetWindowSize(instance, display_bounds.w, display_bounds.h))
        throw sdl_exception("Could not set window size to {}, {} on display {}", display_bounds.w, display_bounds.h,
                            display);
      if (auto center{calculate_display_center(display, static_cast<unsigned int>(display_bounds.w),
                                               static_cast<unsigned int>(display_bounds.h))};
          !SDL_SetWindowPosition(instance, center.x, center.y))
        throw sdl_exception("Could not set window position centered on display {}", display);
      return;
    }
    if (!SDL_SetWindowBordered(instance, true)) throw sdl_exception("Could not set window bordered");
    if (!SDL_SetWindowSize(instance, static_cast<int>(windowed_width), static_cast<int>(windowed_height)))
      throw sdl_exception("Could not set window size to {}, {}", windowed_width, windowed_height);
    auto absolute{relative_to_absolute(display, windowed_left, windowed_top)};
    if (!SDL_SetWindowPosition(instance, absolute.x, absolute.y))
      throw sdl_exception("Could not set window position to {}, {}", absolute.x, absolute.y);
  }

  void window_graphics::handle_vsync()
  {
    if (active.vsync)
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

  void scene_graphics::create(SDL_GPUDevice *gpu)
  {
    const std::array<corner, 4> vertices{
      {{1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, -1.0f, 1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f, 1.0f}, {-1.0f, -1.0f, 0.0f, 0.0f}}};
    const std::array<Uint16, 6> indices{3, 1, 0, 3, 0, 2};
    SDL_GPUBufferCreateInfo vertex_buffer_info{
      .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = sizeof(vertices), .props = 0};
    vertex_buffer = SDL_CreateGPUBuffer(gpu, &vertex_buffer_info);
    if (!vertex_buffer) throw sdl_exception("Could not create vertex buffer for scene");
    SDL_GPUBufferCreateInfo index_buffer_info{.usage = SDL_GPU_BUFFERUSAGE_INDEX, .size = sizeof(indices), .props = 0};
    index_buffer = SDL_CreateGPUBuffer(gpu, &index_buffer_info);
    if (!index_buffer) throw sdl_exception("Could not create index buffer for scene");
    SDL_GPUSamplerCreateInfo sampler_info{};
    sampler_info.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler = SDL_CreateGPUSampler(gpu, &sampler_info);
    if (!sampler) throw sdl_exception("Could not create sampler for scene");
    SDL_GPUTransferBufferCreateInfo transfer_buffer_info{
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = sizeof(vertices) + sizeof(indices), .props = 0};
    auto *transfer_buffer{SDL_CreateGPUTransferBuffer(gpu, &transfer_buffer_info)};
    if (!transfer_buffer) throw sdl_exception("Could not create transfer buffer for scene");
    auto start{static_cast<char *>(SDL_MapGPUTransferBuffer(gpu, transfer_buffer, false))};
    if (!start) throw sdl_exception("Could not map data for scene");
    SDL_memcpy(start, vertices.data(), sizeof(vertices));
    SDL_memcpy(start + sizeof(vertices), indices.data(), sizeof(indices));
    SDL_UnmapGPUTransferBuffer(gpu, transfer_buffer);
    auto *command_buffer{SDL_AcquireGPUCommandBuffer(gpu)};
    if (!command_buffer) throw sdl_exception("Could not acquire GPU command buffer for scene");
    auto *copy_pass{SDL_BeginGPUCopyPass(command_buffer)};
    if (!copy_pass) throw sdl_exception("Could not begin GPU copy pass for scene");
    SDL_GPUTransferBufferLocation vertex_transfer_location{.transfer_buffer = transfer_buffer, .offset = 0};
    SDL_GPUBufferRegion vertex_buffer_region{.buffer = vertex_buffer, .offset = 0, .size = sizeof(vertices)};
    SDL_UploadToGPUBuffer(copy_pass, &vertex_transfer_location, &vertex_buffer_region, false);
    SDL_GPUTransferBufferLocation index_transfer_location{.transfer_buffer = transfer_buffer,
                                                          .offset = sizeof(vertices)};
    SDL_GPUBufferRegion index_buffer_region{.buffer = index_buffer, .offset = 0, .size = sizeof(indices)};
    SDL_UploadToGPUBuffer(copy_pass, &index_transfer_location, &index_buffer_region, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    SDL_ReleaseGPUTransferBuffer(gpu, transfer_buffer);
  }

  void scene_graphics::render(SDL_Window *instance, SDL_GPUDevice *gpu, const camera *camera,
                              const std::vector<std::shared_ptr<object>> &objects,
                              const std::pair<glm::dmat4, glm::dmat4> &matrices, SDL_GPUCommandBuffer *command_buffer,
                              SDL_GPURenderPass *render_pass, const double alpha)
  {
    generate_order(camera, objects, alpha);
    generate_samples_and_batches(instance, gpu, alpha);
    if (stream.samples.empty()) return;
    upload_samples(gpu);
    draw_batches(command_buffer, render_pass, matrices);
  }

  void scene_graphics::destroy(SDL_GPUDevice *gpu)
  {
    SDL_ReleaseGPUTransferBuffer(gpu, stream.transfer_buffer);
    SDL_ReleaseGPUBuffer(gpu, stream.buffer);
    for (const auto &[key, texture] : texture_cache) SDL_ReleaseGPUTexture(gpu, texture);
    for (const auto &[key, available] : pipeline_cache)
    {
      SDL_ReleaseGPUGraphicsPipeline(gpu, available.transparent);
      SDL_ReleaseGPUGraphicsPipeline(gpu, available.opaque);
    }
    SDL_ReleaseGPUSampler(gpu, sampler);
    SDL_ReleaseGPUBuffer(gpu, index_buffer);
    SDL_ReleaseGPUBuffer(gpu, vertex_buffer);
    stream.transfer_buffer = nullptr;
    stream.buffer = nullptr;
    stream.capacity = 0;
    stream.samples.clear();
    stream.batches.clear();
    texture_cache.clear();
    pipeline_cache.clear();
    order.clear();
    sampler = nullptr;
    index_buffer = nullptr;
    vertex_buffer = nullptr;
  }

  void scene_graphics::generate_order(const camera *camera, const std::vector<std::shared_ptr<object>> &objects,
                                      const double alpha)
  {
    order.clear();
    for (order.reserve(objects.size()); const auto &object : objects) order.emplace_back(object.get());
    auto camera_translation =
      camera->state.previous.translation.value +
      (camera->state.active.translation.value - camera->state.previous.translation.value) * alpha;
    auto camera_forward =
      glm::normalize(camera->state.previous.forward.value +
                     (camera->state.active.forward.value - camera->state.previous.forward.value) * alpha);
    std::sort(order.begin(), order.end(),
              [alpha, &camera_translation, &camera_forward](const auto &left, const auto &right)
              {
                double left_depth =
                  glm::dot((left->state.previous.translation.value +
                            (left->state.active.translation.value - left->state.previous.translation.value) * alpha) -
                             camera_translation,
                           camera_forward);
                double right_depth =
                  glm::dot((right->state.previous.translation.value +
                            (right->state.active.translation.value - right->state.previous.translation.value) * alpha) -
                             camera_translation,
                           camera_forward);
                if (!equal(left_depth, right_depth, 1e-4)) return left_depth > right_depth;
                if (left->graphics.active.priority != right->graphics.active.priority)
                  return left->graphics.active.priority < right->graphics.active.priority;
                const auto left_batch{std::make_tuple(left->graphics.active.shader.vertex->source.data(),
                                                      left->graphics.active.shader.fragment->source.data(),
                                                      left->graphics.active.texture.image->data.data())};
                const auto right_batch{std::make_tuple(right->graphics.active.shader.vertex->source.data(),
                                                       right->graphics.active.shader.fragment->source.data(),
                                                       right->graphics.active.texture.image->data.data())};
                if (left_batch != right_batch) return left_batch < right_batch;
                return left->name.identifier() < right->name.identifier();
              });
  }

  void scene_graphics::generate_samples_and_batches(SDL_Window *instance, SDL_GPUDevice *gpu, const double alpha)
  {
    stream.samples.clear();
    stream.batches.clear();
    stream.samples.reserve(order.size());
    for (auto *object : order)
    {
      auto &graphics{object->graphics};
      auto &frame{graphics.active.render.playback.frame};
      const auto size{graphics.active.texture.animation.frames.size()};
      if (size == 0) throw exception("Object '{}' contains no frames", object->name.string());
      if (frame >= size) frame = size - 1;
      const auto &coordinates{graphics.active.texture.animation.frames[frame].coordinates};
      const auto &flip{graphics.active.render.flip};
      const auto color{glm::vec4{graphics.previous.render.color.value +
                                 (graphics.active.render.color.value - graphics.previous.render.color.value) * alpha}};
      const auto transparency{
        graphics.previous.render.transparency.value +
        (graphics.active.render.transparency.value - graphics.previous.render.transparency.value) * alpha};
      const glm::mat4 model{object->state.calculate_model_matrix(graphics.active.texture.image->frame_width,
                                                                 graphics.active.texture.image->frame_height, alpha)};
      sample data{};
      SDL_memcpy(data.model.data(), &model, sizeof(model));
      data.r = color.r;
      data.g = color.g;
      data.b = color.b;
      data.a = color.a;
      data.left = static_cast<float>(flip.horizontal ? coordinates.right : coordinates.left);
      data.right = static_cast<float>(flip.horizontal ? coordinates.left : coordinates.right);
      data.top = static_cast<float>(flip.vertical ? coordinates.bottom : coordinates.top);
      data.bottom = static_cast<float>(flip.vertical ? coordinates.top : coordinates.bottom);
      data.transparency = static_cast<float>(transparency);
      auto &available{
        require_pipelines(instance, gpu, *graphics.active.shader.vertex, *graphics.active.shader.fragment)};
      auto *pipeline{transparency < 1.0 ? available.transparent : available.opaque};
      auto *texture{require_texture(gpu, *graphics.active.texture.image)};
      if (!stream.batches.empty() && stream.batches.back().pipeline == pipeline &&
          stream.batches.back().texture == texture)
        stream.batches.back().count++;
      else
        stream.batches.push_back({stream.samples.size(), 1, pipeline, texture});
      stream.samples.push_back(data);
    }
  }

  void scene_graphics::upload_samples(SDL_GPUDevice *gpu)
  {
    if (stream.samples.size() > stream.capacity)
    {
      SDL_ReleaseGPUTransferBuffer(gpu, stream.transfer_buffer);
      SDL_ReleaseGPUBuffer(gpu, stream.buffer);
      stream.capacity = std::max<std::size_t>(stream.capacity, 16);
      while (stream.capacity < stream.samples.size()) stream.capacity *= 2;
      SDL_GPUBufferCreateInfo instance_buffer_info{
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = static_cast<Uint32>(sizeof(sample) * stream.capacity), .props = 0};
      stream.buffer = SDL_CreateGPUBuffer(gpu, &instance_buffer_info);
      if (!stream.buffer) throw sdl_exception("Could not create instance buffer for scene");
      SDL_GPUTransferBufferCreateInfo instance_transfer_buffer_info{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = static_cast<Uint32>(sizeof(sample) * stream.capacity),
        .props = 0};
      stream.transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &instance_transfer_buffer_info);
      if (!stream.transfer_buffer) throw sdl_exception("Could not create instance transfer buffer for scene");
    }
    auto *start{SDL_MapGPUTransferBuffer(gpu, stream.transfer_buffer, true)};
    if (!start) throw sdl_exception("Could not map instance data for scene");
    SDL_memcpy(start, stream.samples.data(), sizeof(sample) * stream.samples.size());
    SDL_UnmapGPUTransferBuffer(gpu, stream.transfer_buffer);
    auto *command_buffer{SDL_AcquireGPUCommandBuffer(gpu)};
    if (!command_buffer) throw sdl_exception("Could not acquire GPU command buffer for scene");
    auto *copy_pass{SDL_BeginGPUCopyPass(command_buffer)};
    if (!copy_pass) throw sdl_exception("Could not begin GPU copy pass for scene");
    SDL_GPUTransferBufferLocation instance_transfer_location{.transfer_buffer = stream.transfer_buffer, .offset = 0};
    SDL_GPUBufferRegion instance_buffer_region{
      .buffer = stream.buffer, .offset = 0, .size = static_cast<Uint32>(sizeof(sample) * stream.samples.size())};
    SDL_UploadToGPUBuffer(copy_pass, &instance_transfer_location, &instance_buffer_region, true);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
  }

  void scene_graphics::draw_batches(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                                    const std::pair<glm::dmat4, glm::dmat4> &matrices)
  {
    const auto &[projection_matrix, view_matrix] = matrices;
    const std::array<glm::mat4, 2> data{glm::mat4{projection_matrix}, glm::mat4{view_matrix}};
    SDL_PushGPUVertexUniformData(command_buffer, 0, &data, sizeof(data));
    const std::array<SDL_GPUBufferBinding, 2> vertex_buffer_bindings{
      {{.buffer = vertex_buffer, .offset = 0}, {.buffer = stream.buffer, .offset = 0}}};
    SDL_BindGPUVertexBuffers(render_pass, 0, vertex_buffer_bindings.data(), 2);
    SDL_GPUBufferBinding index_buffer_binding{.buffer = index_buffer, .offset = 0};
    SDL_BindGPUIndexBuffer(render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    for (const auto &group : stream.batches)
    {
      SDL_BindGPUGraphicsPipeline(render_pass, group.pipeline);
      SDL_GPUTextureSamplerBinding texture_sampler_binding{.texture = group.texture, .sampler = sampler};
      SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);
      SDL_DrawGPUIndexedPrimitives(render_pass, 6, static_cast<Uint32>(group.count), 0, 0,
                                   static_cast<Uint32>(group.first));
    }
  }

  scene_graphics::pipelines &scene_graphics::require_pipelines(SDL_Window *instance, SDL_GPUDevice *gpu,
                                                               const cse::vertex &vertex, const cse::fragment &fragment)
  {
    const pipeline_key key{vertex.source.data(), vertex.source.size(), fragment.source.data(), fragment.source.size()};
    if (const auto iterator{pipeline_cache.find(key)}; iterator != pipeline_cache.end()) return iterator->second;
    const auto backend_formats{SDL_GetGPUShaderFormats(gpu)};
    if (!(backend_formats & SDL_GPU_SHADERFORMAT_SPIRV))
      throw sdl_exception("No supported vulkan shader formats for scene");
    SDL_GPUShaderCreateInfo vertex_shader_info{.code_size = vertex.source.size(),
                                               .code = vertex.source.data(),
                                               .entrypoint = "main",
                                               .format = SDL_GPU_SHADERFORMAT_SPIRV,
                                               .stage = SDL_GPU_SHADERSTAGE_VERTEX,
                                               .num_samplers = 0,
                                               .num_storage_textures = 0,
                                               .num_storage_buffers = 0,
                                               .num_uniform_buffers = 1,
                                               .props = 0};
    auto *vertex_shader{SDL_CreateGPUShader(gpu, &vertex_shader_info)};
    if (!vertex_shader) throw sdl_exception("Could not create vertex shader for scene");
    SDL_GPUShaderCreateInfo fragment_shader_info{.code_size = fragment.source.size(),
                                                 .code = fragment.source.data(),
                                                 .entrypoint = "main",
                                                 .format = SDL_GPU_SHADERFORMAT_SPIRV,
                                                 .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
                                                 .num_samplers = 1,
                                                 .num_storage_textures = 0,
                                                 .num_storage_buffers = 0,
                                                 .num_uniform_buffers = 0,
                                                 .props = 0};
    auto *fragment_shader{SDL_CreateGPUShader(gpu, &fragment_shader_info)};
    if (!fragment_shader) throw sdl_exception("Could not create fragment shader for scene");
    const std::array<SDL_GPUVertexBufferDescription, 2> vertex_buffer_descriptions{
      {{.slot = 0, .pitch = sizeof(corner), .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX, .instance_step_rate = 0},
       {.slot = 1, .pitch = sizeof(sample), .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE, .instance_step_rate = 0}}};
    const std::array<SDL_GPUVertexAttribute, 9> vertex_attributes{
      {{0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(corner, x)},
       {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(corner, u)},
       {2, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(sample, model)},
       {3, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(sample, model) + sizeof(float) * 4},
       {4, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(sample, model) + sizeof(float) * 8},
       {5, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(sample, model) + sizeof(float) * 12},
       {6, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(sample, r)},
       {7, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(sample, left)},
       {8, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, offsetof(sample, transparency)}}};
    SDL_GPUVertexInputState vertex_input_state{.vertex_buffer_descriptions = vertex_buffer_descriptions.data(),
                                               .num_vertex_buffers = 2,
                                               .vertex_attributes = vertex_attributes.data(),
                                               .num_vertex_attributes = 9};
    SDL_GPURasterizerState rasterizer_state{};
    rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    SDL_GPUColorTargetDescription opaque_color_target_description{};
    opaque_color_target_description.format = SDL_GetGPUSwapchainTextureFormat(gpu, instance);
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
    opaque_target_info.depth_stencil_format = [gpu, &potential_formats]() -> SDL_GPUTextureFormat
    {
      for (const auto &potential_format : potential_formats)
        if (SDL_GPUTextureSupportsFormat(gpu, potential_format, type, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
          return potential_format;
      return {};
    }();
    opaque_target_info.has_depth_stencil_target = true;
    if (opaque_target_info.depth_stencil_format == SDL_GPU_TEXTUREFORMAT_INVALID)
      throw sdl_exception("No supported depth stencil format found for scene");
    SDL_GPUGraphicsPipelineCreateInfo opaque_pipeline_info{};
    opaque_pipeline_info.vertex_shader = vertex_shader;
    opaque_pipeline_info.fragment_shader = fragment_shader;
    opaque_pipeline_info.vertex_input_state = vertex_input_state;
    opaque_pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    opaque_pipeline_info.rasterizer_state = rasterizer_state;
    opaque_pipeline_info.depth_stencil_state = opaque_depth_stencil_state;
    opaque_pipeline_info.target_info = opaque_target_info;
    pipelines available{};
    available.opaque = SDL_CreateGPUGraphicsPipeline(gpu, &opaque_pipeline_info);
    if (!available.opaque) throw sdl_exception("Could not create graphics pipeline for scene");
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
    transparent_color_target_description.format = SDL_GetGPUSwapchainTextureFormat(gpu, instance);
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
    available.transparent = SDL_CreateGPUGraphicsPipeline(gpu, &transparent_pipeline_info);
    if (!available.transparent) throw sdl_exception("Could not create transparent graphics pipeline for scene");
    SDL_ReleaseGPUShader(gpu, fragment_shader);
    SDL_ReleaseGPUShader(gpu, vertex_shader);
    return pipeline_cache.emplace(key, available).first->second;
  }

  SDL_GPUTexture *scene_graphics::require_texture(SDL_GPUDevice *gpu, const cse::image &image)
  {
    const texture_key key{image.data.data(), image.data.size()};
    if (const auto iterator{texture_cache.find(key)}; iterator != texture_cache.end()) return iterator->second;
    const auto type{SDL_GPU_TEXTURETYPE_2D};
    const auto format{SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM};
    const auto usage{SDL_GPU_TEXTUREUSAGE_SAMPLER};
    if (!SDL_GPUTextureSupportsFormat(gpu, format, type, usage))
      throw sdl_exception("No supported texture format found for scene");
    SDL_GPUTextureCreateInfo texture_info{.type = type,
                                          .format = format,
                                          .usage = usage,
                                          .width = image.width,
                                          .height = image.height,
                                          .layer_count_or_depth = 1,
                                          .num_levels = 1,
                                          .sample_count = SDL_GPU_SAMPLECOUNT_1,
                                          .props = 0};
    auto *texture{SDL_CreateGPUTexture(gpu, &texture_info)};
    if (!texture) throw sdl_exception("Could not create texture for scene");
    SDL_GPUTransferBufferCreateInfo texture_transfer_buffer_info{
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = image.width * image.height * image.channels, .props = 0};
    auto *texture_transfer_buffer{SDL_CreateGPUTransferBuffer(gpu, &texture_transfer_buffer_info)};
    if (!texture_transfer_buffer) throw sdl_exception("Could not create transfer buffer for texture for scene");
    auto *texture_data{SDL_MapGPUTransferBuffer(gpu, texture_transfer_buffer, false)};
    if (!texture_data) throw sdl_exception("Could not map texture data for scene");
    SDL_memcpy(texture_data, image.data.data(), image.width * image.height * image.channels);
    SDL_UnmapGPUTransferBuffer(gpu, texture_transfer_buffer);
    auto *command_buffer{SDL_AcquireGPUCommandBuffer(gpu)};
    if (!command_buffer) throw sdl_exception("Could not acquire GPU command buffer for scene");
    auto *copy_pass{SDL_BeginGPUCopyPass(command_buffer)};
    if (!copy_pass) throw sdl_exception("Could not begin GPU copy pass for scene");
    SDL_GPUTextureTransferInfo texture_transfer_info{
      .transfer_buffer = texture_transfer_buffer, .offset = 0, .pixels_per_row = 0, .rows_per_layer = 0};
    SDL_GPUTextureRegion texture_region{.texture = texture,
                                        .mip_level = 0,
                                        .layer = 0,
                                        .x = 0,
                                        .y = 0,
                                        .z = 0,
                                        .w = image.width,
                                        .h = image.height,
                                        .d = 1};
    SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    SDL_ReleaseGPUTransferBuffer(gpu, texture_transfer_buffer);
    return texture_cache.emplace(key, texture).first->second;
  }

  camera_graphics::camera_graphics(const double fov_, const clip &clip_) : previous{fov_, clip_}, active{fov_, clip_} {}

  void camera_graphics::update_previous()
  {
    previous.fov = active.fov;
    previous.clip.near = active.clip.near;
    previous.clip.far = active.clip.far;
  }

  glm::dmat4 camera_graphics::calculate_projection_matrix(const double previous_aspect, const double active_aspect,
                                                          const double alpha)
  {
    return glm::perspective(glm::radians(previous.fov.value + (active.fov.value - previous.fov.value) * alpha),
                            previous_aspect + (active_aspect - previous_aspect) * alpha, active.clip.near,
                            active.clip.far);
  }

  object_graphics::object_graphics(const shader &shader_, const texture &texture_, const render &render_,
                                   const int priority_)
    : previous{shader_, texture_, render_, priority_}, active{shader_, texture_, render_, priority_}
  {
  }

  void object_graphics::update_previous()
  {
    previous.shader.vertex = active.shader.vertex;
    previous.shader.fragment = active.shader.fragment;
    previous.texture.image = active.texture.image;
    previous.texture.animation = active.texture.animation;
    previous.render.playback = active.render.playback;
    previous.render.flip = active.render.flip;
    previous.render.color = active.render.color;
    previous.render.transparency = active.render.transparency;
    previous.priority = active.priority;
  }

  void object_graphics::animate(const double tick)
  {
    auto &animation{active.texture.animation};
    auto &playback{active.render.playback};
    auto no_frames{animation.frames.empty()};
    auto frame_count{animation.frames.size()};
    if (no_frames)
      playback.frame = 0;
    else if (playback.frame >= frame_count)
      playback.frame = frame_count - 1;
    if (playback.speed.value > 0.0 && !no_frames)
    {
      playback.elapsed += tick * playback.speed.value;
      while (true)
      {
        auto duration = animation.frames[playback.frame].duration;
        if (duration > 0 && playback.elapsed < duration) break;
        if (playback.frame < frame_count - 1)
        {
          if (duration > 0) playback.elapsed -= duration;
          playback.frame++;
        }
        else if (playback.loop)
        {
          if (duration > 0)
            playback.elapsed -= duration;
          else
            break;
          playback.frame = 0;
        }
        else
          break;
      }
    }
    else if (playback.speed.value < 0.0 && !no_frames)
    {
      playback.elapsed += tick * playback.speed.value;
      while (playback.elapsed < 0)
        if (playback.frame > 0)
        {
          playback.frame--;
          auto duration = animation.frames[playback.frame].duration;
          if (duration > 0) playback.elapsed += duration;
        }
        else if (playback.loop)
        {
          if (animation.frames[0].duration <= 0) break;
          playback.frame = frame_count - 1;
          auto duration = animation.frames[playback.frame].duration;
          if (duration > 0) playback.elapsed += duration;
        }
        else
          break;
    }
  }
}
