#include "window.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/ext/vector_int2.hpp"

#include "core.hpp"
#include "exception.hpp"
#include "game.hpp"
#include "input.hpp"
#include "log.hpp"
#include "mask.hpp"

namespace cse::help::window
{
  previous::previous(const std::string &title_, const SDL_DisplayID display_, const int left_, const int top_,
                     const unsigned int width_, const unsigned int height_, const ::mode mode_, const bool vsync_,
                     const cse::mouse::initial &mouse_)
    : title{title_}, display{display_}, left{left_}, top{top_}, width{width_}, height{height_}, mode{mode_},
      vsync{vsync_}, mouse{mouse_.visible, mouse_.position}
  {
  }

  active::active(const std::string &title_, const SDL_DisplayID display_, const int left_, const int top_,
                 const unsigned int width_, const unsigned int height_, const ::mode mode_, const bool vsync_,
                 const cse::mouse::initial &mouse_)
    : title{title_}, display{display_}, left{left_}, top{top_}, width{width_}, height{height_}, mode{mode_},
      vsync{vsync_}, mouse{mouse_.visible, mouse_.position}
  {
  }

  void active::create(SDL_GPUDevice *video, const double aspect, const unsigned int resolution)
  {
    instance = SDL_CreateWindow(title.c_str(), static_cast<int>(width), static_cast<int>(height),
                                SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!instance) throw sdl_exception("Could not create window");

    if (display == PRIMARY || !display_exists(display)) display = SDL_GetPrimaryDisplay();
    if (display == SDL_DisplayID{0}) sdl_log("Invalid display index {}", display);

    auto absolute_center{calculate_display_center(width, height)};
    auto relative_center{absolute_to_relative(absolute_center.x, absolute_center.y)};
    left = left == ORIGIN ? relative_center.x : left;
    top = top == ORIGIN ? relative_center.y : top;
    windowed_left = left;
    windowed_top = top;
    auto absolute{relative_to_absolute(left, top)};
    if (can_move() && !SDL_SetWindowPosition(instance, absolute.x, absolute.y))
      sdl_log("Could not set window position to {}, {}", absolute.x, absolute.y);

    const auto pixels{pixel_size()};
    render_width = static_cast<unsigned int>(pixels.x);
    render_height = static_cast<unsigned int>(pixels.y);

    if (!SDL_ClaimWindowForGPUDevice(video, instance)) throw sdl_exception("Could not claim window for GPU device");
    if (!SDL_SetGPUSwapchainParameters(video, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
      sdl_log("Could not enable VSYNC");

    if (mode != WINDOWED) handle_mode();
    if (!vsync) handle_vsync(video);
    if (!depth_texture) generate_depth_texture(video);

    shadow = {.title = title,
              .display = display,
              .left = left,
              .top = top,
              .width = width,
              .height = height,
              .mode = mode,
              .vsync = vsync};
    SDL_ShowWindow(instance);

    if (!mouse.visible) SDL_HideCursor();
    const auto density{pixel_density()};
    const auto pixel{to_pixel(mouse.position.x, mouse.position.y, aspect, resolution)};
    SDL_WarpMouseInWindow(instance, static_cast<float>(pixel.x) / density, static_cast<float>(pixel.y) / density);
    shadow.mouse.position = mouse.position;
  }

  void active::synchronize(previous &last)
  {
    last.title = title;
    last.display = display;
    last.left = left;
    last.top = top;
    last.width = width;
    last.height = height;
    last.mode = mode;
    last.vsync = vsync;

    last.running = running;
    last.keyboard = keyboard;
    last.mouse = mouse;
    last.timer = timer;
    last.mixer = mixer;
    last.phase = phase;

    for (auto &[name, sound] : mixer.sounds)
    {
      sound.speed.instant = false;
      sound.volume.instant = false;
    }
    for (auto &[name, music] : mixer.musics)
    {
      music.speed.instant = false;
      music.volume.instant = false;
    }
    mouse.wheel = {};
  }

  void active::render(const help::game::active &game_active, const double aspect, const glm::dvec3 &clear)
  {
    SDL_GPUColorTargetInfo color_target_info{};
    color_target_info.texture = swapchain_texture;
    color_target_info.clear_color = {static_cast<float>(clear.r), static_cast<float>(clear.g),
                                     static_cast<float>(clear.b), 1.0f};
    color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPUDepthStencilTargetInfo depth_stencil_target_info{};
    depth_stencil_target_info.texture = depth_texture;
    depth_stencil_target_info.clear_depth = 1.0f;
    depth_stencil_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_stencil_target_info.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depth_stencil_target_info.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depth_stencil_target_info.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    depth_stencil_target_info.cycle = true;
    render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, &depth_stencil_target_info);
    if (!render_pass) throw sdl_exception("Could not begin GPU render pass");
    float viewport_left{}, viewport_top{}, viewport_width{}, viewport_height{};
    const auto target_aspect{static_cast<float>(aspect)};
    if ((static_cast<float>(render_width) / static_cast<float>(render_height)) > target_aspect)
    {
      viewport_height = static_cast<float>(render_height);
      viewport_width = viewport_height * target_aspect;
      viewport_top = 0.0f;
      viewport_left = (static_cast<float>(render_width) - viewport_width) / 2.0f;
    }
    else
    {
      viewport_width = static_cast<float>(render_width);
      viewport_height = viewport_width / target_aspect;
      viewport_left = 0.0f;
      viewport_top = (static_cast<float>(render_height) - viewport_height) / 2.0f;
    }
    SDL_GPUViewport port{.x = viewport_left,
                         .y = viewport_top,
                         .w = viewport_width,
                         .h = viewport_height,
                         .min_depth = 0.0f,
                         .max_depth = 1.0f};
    SDL_SetGPUViewport(render_pass, &port);

    if (!game_active.graphics_object.batches.empty())
    {
      const std::array<SDL_GPUBufferBinding, 2> vertex_buffer_bindings{
        {{.buffer = game_active.graphics_buffer.vertex, .offset = 0},
         {.buffer = game_active.graphics_object.buffer, .offset = 0}}};
      SDL_BindGPUVertexBuffers(render_pass, 0, vertex_buffer_bindings.data(), 2);
      SDL_GPUBufferBinding index_buffer_binding{.buffer = game_active.graphics_buffer.index, .offset = 0};
      SDL_BindGPUIndexBuffer(render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
      const std::array<SDL_GPUBuffer *, 2> storage_buffers{game_active.graphics_light.buffer,
                                                           game_active.graphics_occluder.buffer};
      SDL_BindGPUFragmentStorageBuffers(render_pass, 0, storage_buffers.data(), 2);
      SDL_GPUTextureSamplerBinding occluder_binding{.texture = game_active.graphics_occluder.texture,
                                                    .sampler = game_active.graphics_buffer.linear};
      SDL_BindGPUFragmentSamplers(render_pass, 1, &occluder_binding, 1);
      SDL_GPUGraphicsPipeline *pipeline{};
      SDL_GPUTexture *texture{};
      const auto draw_range{[&](const std::size_t first, const std::size_t last)
                            {
                              for (std::size_t index{first}; index < last; ++index)
                              {
                                const auto &group{game_active.graphics_object.batches[index]};
                                if (group.pipeline != pipeline)
                                {
                                  SDL_BindGPUGraphicsPipeline(render_pass, group.pipeline);
                                  pipeline = group.pipeline;
                                }
                                if (group.texture != texture)
                                {
                                  SDL_GPUTextureSamplerBinding texture_binding{
                                    .texture = group.texture, .sampler = game_active.graphics_buffer.nearest};
                                  SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_binding, 1);
                                  texture = group.texture;
                                }
                                SDL_DrawGPUIndexedPrimitives(render_pass, 6, static_cast<Uint32>(group.count), 0, 0,
                                                             static_cast<Uint32>(group.first));
                              }
                            }};
      if (game_active.graphics_object.split > 0)
      {
        const std::array<glm::mat4, 2> matrices{glm::mat4{game_active.graphics_object.world.first},
                                                glm::mat4{game_active.graphics_object.world.second}};
        SDL_PushGPUVertexUniformData(command_buffer, 0, &matrices, sizeof(matrices));
        SDL_PushGPUFragmentUniformData(command_buffer, 0, &game_active.graphics_light.data,
                                       sizeof(game_active.graphics_light.data));
        draw_range(0, game_active.graphics_object.split);
      }
      if (game_active.graphics_object.split < game_active.graphics_object.batches.size())
      {
        const std::array<glm::mat4, 2> matrices{glm::mat4{game_active.graphics_object.overlay.first},
                                                glm::mat4{game_active.graphics_object.overlay.second}};
        auto overlay_data{game_active.graphics_light.data};
        overlay_data.meta[0] = 0.0f;
        overlay_data.meta[1] = 0.0f;
        SDL_PushGPUVertexUniformData(command_buffer, 0, &matrices, sizeof(matrices));
        SDL_PushGPUFragmentUniformData(command_buffer, 0, &overlay_data, sizeof(overlay_data));
        draw_range(game_active.graphics_object.split, game_active.graphics_object.batches.size());
      }
    }

    SDL_EndGPURenderPass(render_pass);
    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) throw sdl_exception("Could not submit GPU command buffer");
  }

  void active::destroy(SDL_GPUDevice *video)
  {
    SDL_ReleaseGPUTexture(video, depth_texture);
    SDL_ReleaseWindowFromGPUDevice(video, instance);
    SDL_DestroyWindow(instance);
  }

  void active::poll(const double aspect, const unsigned int resolution)
  {
    float x{}, y{};
    const auto buttons{SDL_GetMouseState(&x, &y)};
    const auto density{pixel_density()};
    x *= density;
    y *= density;
    const auto view{letterbox(aspect)};
    const bool warping{mouse.position != shadow.mouse.position};
    const bool focused{SDL_GetMouseFocus() == instance};
    const bool inside{focused && x >= view.left && x <= view.left + view.width && y >= view.top &&
                      y <= view.top + view.height};
    const bool show{mouse.visible || (!warping && !inside)};

    if (show && !SDL_CursorVisible())
      SDL_ShowCursor();
    else if (!show && SDL_CursorVisible())
      SDL_HideCursor();
    for (std::size_t button{SDL_BUTTON_LEFT}; button <= SDL_BUTTON_X2; ++button)
      mouse.buttons[button] = has(buttons, SDL_BUTTON_MASK(button));
    if (warping)
    {
      const auto pixel{to_pixel(mouse.position.x, mouse.position.y, aspect, resolution)};
      SDL_WarpMouseInWindow(instance, static_cast<float>(pixel.x) / density, static_cast<float>(pixel.y) / density);
    }
    else
      mouse.position = to_virtual(x, y, aspect, resolution);
    shadow.mouse.position = mouse.position;

    std::copy_n(SDL_GetKeyboardState(nullptr), keyboard.size(), keyboard.begin());
  }

  active::viewport active::letterbox(const double aspect)
  {
    const auto window_width{static_cast<double>(render_width)};
    const auto window_height{static_cast<double>(render_height)};
    viewport result{};
    if (window_width / window_height > aspect)
    {
      result.height = window_height;
      result.width = result.height * aspect;
      result.left = (window_width - result.width) / 2.0;
    }
    else
    {
      result.width = window_width;
      result.height = result.width / aspect;
      result.top = (window_height - result.height) / 2.0;
    }
    return result;
  }

  float active::pixel_density()
  {
    const auto density{SDL_GetWindowPixelDensity(instance)};
    return density > 0.0f ? density : 1.0f;
  }

  glm::ivec2 active::pixel_size()
  {
    glm::ivec2 size{};
    if (!SDL_GetWindowSizeInPixels(instance, &size.x, &size.y))
    {
      sdl_log("Could not get window size in pixels");
      size = {static_cast<int>(width), static_cast<int>(height)};
    }
    if (size.x <= 0) size.x = 1;
    if (size.y <= 0) size.y = 1;
    return size;
  }

  glm::dvec2 active::to_virtual(const double x, const double y, const double aspect, const unsigned int resolution)
  {
    const auto view{letterbox(aspect)};
    const auto canvas_height{static_cast<double>(std::max(1u, resolution))};
    const auto canvas_width{canvas_height * aspect};
    const glm::dvec2 canvas{(x - view.left) / view.width * canvas_width - canvas_width / 2.0,
                            (y - view.top) / view.height * canvas_height - canvas_height / 2.0};
    return {canvas.x + (std::llround(canvas_width) % 2 == 0 ? 0.5 : 0.0),
            -(canvas.y + (std::llround(canvas_height) % 2 == 0 ? 0.5 : 0.0))};
  }

  glm::dvec2 active::to_pixel(const double x, const double y, const double aspect, const unsigned int resolution)
  {
    const auto view{letterbox(aspect)};
    const auto canvas_height{static_cast<double>(std::max(1u, resolution))};
    const auto canvas_width{canvas_height * aspect};
    const glm::dvec2 canvas{x - (std::llround(canvas_width) % 2 == 0 ? 0.5 : 0.0),
                            -y - (std::llround(canvas_height) % 2 == 0 ? 0.5 : 0.0)};
    return {(canvas.x + canvas_width / 2.0) / canvas_width * view.width + view.left,
            (canvas.y + canvas_height / 2.0) / canvas_height * view.height + view.top};
  }

  void active::reconcile(SDL_GPUDevice *video)
  {
    if (title != shadow.title) handle_title_change();
    if (left != shadow.left || top != shadow.top)
      handle_manual_move();
    else if (display != shadow.display)
      handle_manual_display_move();
    if (width != shadow.width || height != shadow.height) handle_manual_resize(video);
    if (mode != shadow.mode) handle_mode();
    if (vsync != shadow.vsync) handle_vsync(video);
    shadow.title = title;
    shadow.display = display;
    shadow.left = left;
    shadow.top = top;
    shadow.width = width;
    shadow.height = height;
    shadow.mode = mode;
    shadow.vsync = vsync;
  }

  void active::generate_depth_texture(SDL_GPUDevice *video)
  {
    if (depth_texture)
    {
      SDL_ReleaseGPUTexture(video, depth_texture);
      depth_texture = nullptr;
    }
    const auto type{SDL_GPU_TEXTURETYPE_2D};
    const auto usage{SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET};
    const std::array<SDL_GPUTextureFormat, 3> potential_formats{
      SDL_GPU_TEXTUREFORMAT_D32_FLOAT, SDL_GPU_TEXTUREFORMAT_D24_UNORM, SDL_GPU_TEXTUREFORMAT_D16_UNORM};
    SDL_GPUTextureCreateInfo depth_texture_info{
      .type = type,
      .format = [&video, &potential_formats]() -> SDL_GPUTextureFormat
      {
        for (const auto &potential_format : potential_formats)
          if (SDL_GPUTextureSupportsFormat(video, potential_format, type, usage)) return potential_format;
        return {};
      }(),
      .usage = usage,
      .width = render_width,
      .height = render_height,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = SDL_GPU_SAMPLECOUNT_1,
      .props = 0};
    if (depth_texture_info.format == SDL_GPU_TEXTUREFORMAT_INVALID)
      throw sdl_exception("No supported depth texture format found");
    depth_texture = SDL_CreateGPUTexture(video, &depth_texture_info);
    if (!depth_texture) throw sdl_exception("Could not create depth texture");
  }

  bool active::acquire_swapchain_texture(SDL_GPUDevice *video)
  {
    command_buffer = SDL_AcquireGPUCommandBuffer(video);
    if (!command_buffer) throw sdl_exception("Could not acquire GPU command buffer");
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, instance, &swapchain_texture, nullptr, nullptr))
    {
      sdl_log("Could not acquire GPU swapchain texture; skipping frame");
      if (!SDL_CancelGPUCommandBuffer(command_buffer)) sdl_log("Could not cancel GPU command buffer");
      return false;
    }
    if (!swapchain_texture)
    {
      if (!SDL_SubmitGPUCommandBuffer(command_buffer)) throw sdl_exception("Could not submit GPU command buffer");
      return false;
    }
    return true;
  }

  bool active::can_move()
  {
    const char *driver{SDL_GetCurrentVideoDriver()};
    if (!driver)
    {
      sdl_log("Could not get current video driver");
      return false;
    }
    return SDL_strcmp(driver, "windows") == 0 || SDL_strcmp(driver, "x11") == 0;
  }

  bool active::display_exists(const SDL_DisplayID target)
  {
    int count{};
    auto *displays{SDL_GetDisplays(&count)};
    if (!displays)
    {
      sdl_log("Could not get displays");
      return false;
    }
    bool found{false};
    for (int index{}; index < count; ++index)
      if (displays[index] == target) found = true;
    SDL_free(displays);
    return found;
  }

  void active::handle_title_change()
  {
    if (!SDL_SetWindowTitle(instance, title.c_str())) sdl_log("Could not set window title");
  }

  void active::handle_move()
  {
    if (mode != WINDOWED) return;
    const auto new_display{SDL_GetDisplayForWindow(instance)};
    if (new_display == SDL_DisplayID{0})
    {
      sdl_log("Could not get window display index");
      return;
    }
    display = new_display;
    shadow.display = display;
    glm::ivec2 absolute{};
    if (!SDL_GetWindowPosition(instance, &absolute.x, &absolute.y))
    {
      sdl_log("Could not get window position");
      return;
    }
    auto relative{absolute_to_relative(absolute.x, absolute.y)};
    left = relative.x;
    top = relative.y;
    windowed_left = left;
    windowed_top = top;
    shadow.display = display;
    shadow.left = left;
    shadow.top = top;
  }

  void active::handle_resize(SDL_GPUDevice *video)
  {
    if (auto new_display = SDL_GetDisplayForWindow(instance); display != new_display)
    {
      if (new_display == SDL_DisplayID{0})
        sdl_log("Could not get window display index");
      else
      {
        display = new_display;
        glm::ivec2 absolute{};
        if (!SDL_GetWindowPosition(instance, &absolute.x, &absolute.y))
          sdl_log("Could not get window position");
        else
        {
          auto relative{absolute_to_relative(absolute.x, absolute.y)};
          left = relative.x;
          top = relative.y;
          if (mode == WINDOWED)
          {
            windowed_left = left;
            windowed_top = top;
          }
        }
      }
    }
    int current_width{}, current_height{};
    SDL_GetWindowSize(instance, &current_width, &current_height);
    if (current_width <= 0) current_width = 1;
    if (current_height <= 0) current_height = 1;
    if (mode == WINDOWED)
    {
      width = static_cast<unsigned int>(current_width);
      height = static_cast<unsigned int>(current_height);
    }
    const auto pixels{pixel_size()};
    render_width = static_cast<unsigned int>(pixels.x);
    render_height = static_cast<unsigned int>(pixels.y);
    generate_depth_texture(video);
    shadow.display = display;
    shadow.left = left;
    shadow.top = top;
    shadow.width = width;
    shadow.height = height;
  }

  void active::handle_manual_display_move()
  {
    if (display == PRIMARY || !display_exists(display)) display = SDL_GetPrimaryDisplay();
    if (display == SDL_DisplayID{0})
    {
      sdl_log("Invalid display index {}", display);
      display = shadow.display;
      return;
    }
    auto absolute_center{calculate_display_center(width, height)};
    auto relative_center(absolute_to_relative(absolute_center.x, absolute_center.y));
    left = relative_center.x;
    top = relative_center.y;
    if (mode == WINDOWED)
    {
      windowed_left = left;
      windowed_top = top;
    }
    auto absolute{relative_to_absolute(left, top)};
    if (can_move() && !SDL_SetWindowPosition(instance, absolute.x, absolute.y))
      sdl_log("Could not set window position centered on display {}", display);
  }

  void active::handle_manual_move()
  {
    if (display == PRIMARY || !display_exists(display)) display = SDL_GetPrimaryDisplay();
    if (display == SDL_DisplayID{0})
    {
      sdl_log("Invalid display index {}", display);
      display = shadow.display;
      return;
    }
    auto absolute_center{calculate_display_center(width, height)};
    auto relative_center(absolute_to_relative(absolute_center.x, absolute_center.y));
    left = left == ORIGIN ? relative_center.x : left;
    top = top == ORIGIN ? relative_center.y : top;
    windowed_left = left;
    windowed_top = top;
    auto absolute{relative_to_absolute(left, top)};
    if (can_move() && !SDL_SetWindowPosition(instance, absolute.x, absolute.y))
      sdl_log("Could not set window position to {}, {}", left, top);
    if (auto new_display = SDL_GetDisplayForWindow(instance); display != new_display)
    {
      const auto previous_display{display};
      display = new_display;
      if (display == PRIMARY || !display_exists(display)) display = SDL_GetPrimaryDisplay();
      if (display == SDL_DisplayID{0})
      {
        sdl_log("Could not get window display index");
        display = previous_display;
        return;
      }
      auto relative{absolute_to_relative(absolute.x, absolute.y)};
      left = relative.x;
      top = relative.y;
      if (mode == WINDOWED)
      {
        windowed_left = left;
        windowed_top = top;
      }
    }
  }

  void active::handle_manual_resize(SDL_GPUDevice *video)
  {
    if (!SDL_SetWindowSize(instance, static_cast<int>(width), static_cast<int>(height)))
    {
      sdl_log("Could not set window size to {}, {}", width, height);
      width = shadow.width;
      height = shadow.height;
      return;
    }
    if (auto new_display = SDL_GetDisplayForWindow(instance); display != new_display)
    {
      const auto previous_display{display};
      display = new_display;
      if (display == PRIMARY || !display_exists(display)) display = SDL_GetPrimaryDisplay();
      if (display == SDL_DisplayID{0})
      {
        sdl_log("Could not get window display index");
        display = previous_display;
      }
      else
      {
        glm::ivec2 absolute{};
        if (!SDL_GetWindowPosition(instance, &absolute.x, &absolute.y))
          sdl_log("Could not get window position");
        else
        {
          auto relative{absolute_to_relative(absolute.x, absolute.y)};
          left = relative.x;
          top = relative.y;
          if (mode == WINDOWED)
          {
            windowed_left = left;
            windowed_top = top;
          }
        }
      }
    }
    if (mode == WINDOWED)
    {
      const auto pixels{pixel_size()};
      render_width = static_cast<unsigned int>(pixels.x);
      render_height = static_cast<unsigned int>(pixels.y);
    }
    generate_depth_texture(video);
  }

  void active::handle_mode()
  {
    if (display == PRIMARY || !display_exists(display)) display = SDL_GetPrimaryDisplay();
    if (display == SDL_DisplayID{0})
    {
      sdl_log("Invalid display index {}; keeping current window mode", display);
      display = shadow.display;
      mode = shadow.mode;
      return;
    }
    if (mode == FULLSCREEN)
    {
      const SDL_DisplayMode *display_mode{SDL_GetDesktopDisplayMode(display)};
      if (!display_mode)
        sdl_log("Could not get desktop display mode for display {}; falling back to borderless fullscreen", display);
      else if (!SDL_SetWindowFullscreenMode(instance, display_mode))
        sdl_log("Could not set exclusive fullscreen mode for display {}; falling back to borderless fullscreen",
                display);
      else if (!SDL_SetWindowFullscreen(instance, true))
        sdl_log("Could not enter exclusive fullscreen; falling back to borderless fullscreen");
      else
        return;
      mode = BORDERLESS;
    }
    if (mode == BORDERLESS)
    {
      if (!SDL_SetWindowFullscreenMode(instance, nullptr))
        sdl_log("Could not set borderless fullscreen mode; falling back to windowed");
      else if (!SDL_SetWindowFullscreen(instance, true))
        sdl_log("Could not enter borderless fullscreen; falling back to windowed");
      else
        return;
      mode = WINDOWED;
    }
    if (!SDL_SetWindowFullscreen(instance, false)) sdl_log("Could not leave fullscreen");
    if (!SDL_SetWindowSize(instance, static_cast<int>(width), static_cast<int>(height)))
      sdl_log("Could not set window size to {}, {}", width, height);
    if (auto absolute{relative_to_absolute(windowed_left, windowed_top)};
        can_move() && !SDL_SetWindowPosition(instance, absolute.x, absolute.y))
      sdl_log("Could not set window position to {}, {}", absolute.x, absolute.y);
  }

  void active::handle_vsync(SDL_GPUDevice *video)
  {
    if (vsync)
    {
      if (!SDL_SetGPUSwapchainParameters(video, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
        sdl_log("Could not enable VSYNC");
      return;
    }
    if (SDL_WindowSupportsGPUPresentMode(video, instance, SDL_GPU_PRESENTMODE_IMMEDIATE))
    {
      if (!SDL_SetGPUSwapchainParameters(video, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                         SDL_GPU_PRESENTMODE_IMMEDIATE))
        sdl_log("Could not disable VSYNC");
      return;
    }
    if (SDL_WindowSupportsGPUPresentMode(video, instance, SDL_GPU_PRESENTMODE_MAILBOX))
    {
      if (!SDL_SetGPUSwapchainParameters(video, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                         SDL_GPU_PRESENTMODE_MAILBOX))
        sdl_log("Could not disable VSYNC");
      return;
    }
    log("Could not disable VSYNC; no uncapped present mode is supported");
  }

  glm::ivec2 active::calculate_display_center(const unsigned int w, const unsigned int h)
  {
    SDL_Rect bounds{};
    if (!SDL_GetDisplayBounds(display, &bounds))
    {
      sdl_log("Could not get bounds for display {}", display);
      return {0, 0};
    }
    return {bounds.x + (bounds.w - static_cast<int>(w)) / 2, bounds.y + (bounds.h - static_cast<int>(h)) / 2};
  }

  glm::ivec2 active::relative_to_absolute(const int x, const int y)
  {
    SDL_Rect bounds{};
    if (!SDL_GetDisplayBounds(display, &bounds))
    {
      sdl_log("Could not get bounds for display {}", display);
      return {x, y};
    }
    return {x + bounds.x, y + bounds.y};
  }

  glm::ivec2 active::absolute_to_relative(const int x, const int y)
  {
    SDL_Rect bounds{};
    if (!SDL_GetDisplayBounds(display, &bounds))
    {
      sdl_log("Could not get bounds for display {}", display);
      return {x, y};
    }
    return {x - bounds.x, y - bounds.y};
  }
}

namespace cse
{
  window::window(const initial &initial_)
    : previous{initial_.title,  initial_.display, initial_.left,  initial_.top,  initial_.width,
               initial_.height, initial_.mode,    initial_.vsync, initial_.mouse},
      active{initial_.title,  initial_.display, initial_.left,  initial_.top,  initial_.width,
             initial_.height, initial_.mode,    initial_.vsync, initial_.mouse}
  {
  }

  void window::on_prepare() {}
  void window::prepare()
  {
    if (active.phase != help::phase::CLEANED) throw exception("Window must be cleaned before preparation");
    active.running = true;
    active.phase = help::phase::PREPARED;
    on_prepare();
  }

  void window::on_create() {}
  void window::create(SDL_GPUDevice *video, const double aspect, const unsigned int resolution)
  {
    if (active.phase != help::phase::PREPARED) throw exception("Window must be prepared before creation");
    active.create(video, aspect, resolution);
    active.phase = help::phase::CREATED;
    on_create();
    active.reconcile(video);
  }

  void window::on_synchronize() {}
  void window::synchronize()
  {
    if (active.phase != help::phase::CREATED) throw exception("Window must be created before synchronization");
    active.synchronize(previous);
    on_synchronize();
  }

  void window::on_event(const SDL_Event &) {}
  void window::event(SDL_GPUDevice *video)
  {
    if (active.phase != help::phase::CREATED) throw exception("Window must be created before processing events");
    switch (active.event.type)
    {
      case SDL_EVENT_QUIT: active.running = false; break;
      case SDL_EVENT_WINDOW_MOVED: active.handle_move(); break;
      case SDL_EVENT_WINDOW_RESIZED: active.handle_resize(video); break;
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: active.handle_resize(video); break;
      case SDL_EVENT_MOUSE_WHEEL:
        active.mouse.wheel += glm::dvec2{active.event.wheel.x, active.event.wheel.y};
        on_event(active.event);
        break;
      default: on_event(active.event); break;
    }
  }

  void window::on_simulate(const double) {}
  void window::simulate(const double tick)
  {
    if (active.phase != help::phase::CREATED) throw exception("Window must be created before simulation");
    active.timer.update(tick);
    on_simulate(tick);
  }

  void window::on_render(const double) {}
  void window::render(const double aspect, const glm::dvec3 &clear, const double alpha)
  {
    if (active.phase != help::phase::CREATED) throw exception("Window must be created before post-rendering");
    active.render(game->active, aspect, clear);
    on_render(alpha);
  }

  void window::on_destroy() {}
  void window::destroy(SDL_GPUDevice *video)
  {
    if (active.phase != help::phase::CREATED) throw exception("Window must be created before destruction");
    active.event = {};
    active.destroy(video);
    active.phase = help::phase::PREPARED;
    on_destroy();
  }

  void window::on_clean() {}
  void window::clean()
  {
    if (active.phase != help::phase::PREPARED) throw exception("Window must be prepared before cleaning");
    active.phase = help::phase::CLEANED;
    on_clean();
  }

  bool window::available(SDL_GPUDevice *video)
  {
    if (active.phase != help::phase::CREATED) throw exception("Window must be created before pre-rendering");
    active.reconcile(video);
    if (!active.acquire_swapchain_texture(video)) return false;
    return true;
  }
}
