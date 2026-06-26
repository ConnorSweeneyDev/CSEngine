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
#include "SDL3/SDL_video.h"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/ext/vector_int2.hpp"

#include "core.hpp"
#include "exception.hpp"
#include "input.hpp"
#include "mask.hpp"

namespace cse::help::window
{
  previous::previous(const std::string &title_, const SDL_DisplayID display_, const int left_, const int top_,
                     const unsigned int width_, const unsigned int height_, const bool fullscreen_, const bool vsync_,
                     const cse::mouse::initial &mouse_)
    : title{title_}, display{display_}, left{left_}, top{top_}, width{width_}, height{height_}, fullscreen{fullscreen_},
      vsync{vsync_}, mouse{mouse_.visible, mouse_.position}
  {
  }

  active::active(const std::string &title_, const SDL_DisplayID display_, const int left_, const int top_,
                 const unsigned int width_, const unsigned int height_, const bool fullscreen_, const bool vsync_,
                 const cse::mouse::initial &mouse_)
    : title{title_}, display{display_}, left{left_}, top{top_}, width{width_}, height{height_}, fullscreen{fullscreen_},
      vsync{vsync_}, mouse{mouse_.visible, mouse_.position}
  {
  }

  void active::create(SDL_GPUDevice *device, const double aspect, const unsigned int resolution,
                      const SDL_DisplayID PRIMARY, const int CENTER)
  {
    instance = SDL_CreateWindow(title.c_str(), static_cast<int>(width), static_cast<int>(height),
                                SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
    if (!instance) throw sdl_exception("Could not create window");

    int display_count{};
    SDL_GetDisplays(&display_count);
    if (display == PRIMARY || display > static_cast<unsigned int>(display_count)) display = SDL_GetPrimaryDisplay();
    if (display == 0) throw sdl_exception("Invalid display index {}", display);

    auto absolute_center{calculate_display_center(width, height)};
    auto relative_center{absolute_to_relative(absolute_center.x, absolute_center.y)};
    left = left == CENTER ? relative_center.x : left;
    top = top == CENTER ? relative_center.y : top;
    windowed_left = left;
    windowed_top = top;
    auto absolute{relative_to_absolute(left, top)};
    if (!SDL_SetWindowPosition(instance, absolute.x, absolute.y))
      throw sdl_exception("Could not set window position to {}, {}", absolute.x, absolute.y);

    windowed_width = width;
    windowed_height = height;

    if (!SDL_ClaimWindowForGPUDevice(device, instance)) throw sdl_exception("Could not claim window for GPU device");
    if (!SDL_SetGPUSwapchainParameters(device, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
      throw sdl_exception("Could not enable VSYNC");

    if (fullscreen) handle_fullscreen();
    if (!vsync) handle_vsync(device);
    if (!depth_texture) generate_depth_texture(device);

    shadow = {.title = title,
              .display = display,
              .left = left,
              .top = top,
              .width = width,
              .height = height,
              .fullscreen = fullscreen,
              .vsync = vsync};
    SDL_ShowWindow(instance);

    if (!mouse.visible) SDL_HideCursor();
    const auto pixel{to_pixel(mouse.position.x, mouse.position.y, aspect, resolution)};
    SDL_WarpMouseInWindow(instance, static_cast<float>(pixel.x), static_cast<float>(pixel.y));
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
    last.fullscreen = fullscreen;
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

  void active::start_render_pass(const double aspect, const glm::dvec3 &clear)
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
    depth_stencil_target_info.store_op = SDL_GPU_STOREOP_STORE;
    render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, &depth_stencil_target_info);
    if (!render_pass) throw sdl_exception("Could not begin GPU render pass");
    float viewport_left{}, viewport_top{}, viewport_width{}, viewport_height{};
    const auto target_aspect{static_cast<float>(aspect)};
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

  void active::end_render_pass()
  {
    SDL_EndGPURenderPass(render_pass);
    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) throw sdl_exception("Could not submit GPU command buffer");
  }

  void active::destroy(SDL_GPUDevice *device)
  {
    SDL_ReleaseGPUTexture(device, depth_texture);
    SDL_ReleaseWindowFromGPUDevice(device, instance);
    SDL_DestroyWindow(instance);
  }

  void active::poll(const double aspect, const unsigned int resolution)
  {
    float x{}, y{};
    const auto buttons{SDL_GetMouseState(&x, &y)};
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
      SDL_WarpMouseInWindow(instance, static_cast<float>(pixel.x), static_cast<float>(pixel.y));
    }
    else
      mouse.position = to_virtual(x, y, aspect, resolution);
    shadow.mouse.position = mouse.position;

    std::copy_n(SDL_GetKeyboardState(nullptr), keyboard.size(), keyboard.begin());
  }

  active::viewport active::letterbox(const double aspect)
  {
    const auto window_width{static_cast<double>(width)};
    const auto window_height{static_cast<double>(height)};
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

  glm::dvec2 active::to_virtual(const double x, const double y, const double aspect, const unsigned int resolution)
  {
    const auto view{letterbox(aspect)};
    const auto canvas_height{static_cast<double>(std::max(1u, resolution))};
    const auto canvas_width{canvas_height * aspect};
    const glm::dvec2 canvas{(x - view.left) / view.width * canvas_width - canvas_width / 2.0,
                            (y - view.top) / view.height * canvas_height - canvas_height / 2.0};
    return {canvas.x + (std::llround(canvas_width) % 2 == 0 ? 0.5 : 0.0),
            canvas.y + (std::llround(canvas_height) % 2 == 0 ? 0.5 : 0.0)};
  }

  glm::dvec2 active::to_pixel(const double x, const double y, const double aspect, const unsigned int resolution)
  {
    const auto view{letterbox(aspect)};
    const auto canvas_height{static_cast<double>(std::max(1u, resolution))};
    const auto canvas_width{canvas_height * aspect};
    const glm::dvec2 canvas{x - (std::llround(canvas_width) % 2 == 0 ? 0.5 : 0.0),
                            y - (std::llround(canvas_height) % 2 == 0 ? 0.5 : 0.0)};
    return {(canvas.x + canvas_width / 2.0) / canvas_width * view.width + view.left,
            (canvas.y + canvas_height / 2.0) / canvas_height * view.height + view.top};
  }

  void active::reconcile(SDL_GPUDevice *device, const SDL_DisplayID PRIMARY, const int CENTER)
  {
    if (title != shadow.title) handle_title_change();
    if (display != shadow.display)
      handle_manual_display_move(PRIMARY);
    else if (left != shadow.left || top != shadow.top)
      handle_manual_move(CENTER);
    if (width != shadow.width || height != shadow.height) handle_manual_resize(device);
    if (fullscreen != shadow.fullscreen) handle_fullscreen();
    if (vsync != shadow.vsync) handle_vsync(device);
    shadow.title = title;
    shadow.display = display;
    shadow.left = left;
    shadow.top = top;
    shadow.width = width;
    shadow.height = height;
    shadow.fullscreen = fullscreen;
    shadow.vsync = vsync;
  }

  void active::generate_depth_texture(SDL_GPUDevice *device)
  {
    if (depth_texture)
    {
      SDL_ReleaseGPUTexture(device, depth_texture);
      depth_texture = nullptr;
    }
    const auto type{SDL_GPU_TEXTURETYPE_2D};
    const auto usage{SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET};
    const std::array<SDL_GPUTextureFormat, 3> potential_formats{
      SDL_GPU_TEXTUREFORMAT_D32_FLOAT, SDL_GPU_TEXTUREFORMAT_D24_UNORM, SDL_GPU_TEXTUREFORMAT_D16_UNORM};
    SDL_GPUTextureCreateInfo depth_texture_info{
      .type = type,
      .format = [&device, &potential_formats]() -> SDL_GPUTextureFormat
      {
        for (const auto &potential_format : potential_formats)
          if (SDL_GPUTextureSupportsFormat(device, potential_format, type, usage)) return potential_format;
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
    depth_texture = SDL_CreateGPUTexture(device, &depth_texture_info);
    if (!depth_texture) throw sdl_exception("Could not create depth texture");
  }

  bool active::acquire_swapchain_texture(SDL_GPUDevice *device)
  {
    command_buffer = SDL_AcquireGPUCommandBuffer(device);
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

  void active::handle_title_change()
  {
    if (!SDL_SetWindowTitle(instance, title.c_str())) throw sdl_exception("Could not set window title");
  }

  void active::handle_move()
  {
    if (fullscreen) return;
    display = SDL_GetDisplayForWindow(instance);
    if (display == SDL_DisplayID{0}) throw sdl_exception("Could not get window display index");
    glm::ivec2 absolute{};
    if (!SDL_GetWindowPosition(instance, &absolute.x, &absolute.y))
      throw sdl_exception("Could not get window position");
    auto relative{absolute_to_relative(absolute.x, absolute.y)};
    left = relative.x;
    top = relative.y;
    windowed_left = left;
    windowed_top = top;
    shadow.display = display;
    shadow.left = left;
    shadow.top = top;
  }

  void active::handle_resize(SDL_GPUDevice *device)
  {
    if (auto new_display = SDL_GetDisplayForWindow(instance); display != new_display)
    {
      display = new_display;
      if (display == SDL_DisplayID{0}) throw sdl_exception("Could not get window display index");
      glm::ivec2 absolute{};
      if (!SDL_GetWindowPosition(instance, &absolute.x, &absolute.y))
        throw sdl_exception("Could not get window position");
      auto relative{absolute_to_relative(absolute.x, absolute.y)};
      left = relative.x;
      top = relative.y;
      if (!fullscreen)
      {
        windowed_left = left;
        windowed_top = top;
      }
    }
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
    generate_depth_texture(device);
    shadow.display = display;
    shadow.left = left;
    shadow.top = top;
    shadow.width = width;
    shadow.height = height;
  }

  void active::handle_manual_display_move(const SDL_DisplayID PRIMARY)
  {
    if (display == PRIMARY) display = SDL_GetPrimaryDisplay();
    auto absolute_center{calculate_display_center(width, height)};
    auto relative_center(absolute_to_relative(absolute_center.x, absolute_center.y));
    left = relative_center.x;
    top = relative_center.y;
    if (!fullscreen)
    {
      windowed_left = left;
      windowed_top = top;
    }
    auto absolute{relative_to_absolute(left, top)};
    if (!SDL_SetWindowPosition(instance, absolute.x, absolute.y))
      throw sdl_exception("Could not set window position centered on display {}", display);
  }

  void active::handle_manual_move(const int CENTER)
  {
    auto absolute_center{calculate_display_center(width, height)};
    auto relative_center(absolute_to_relative(absolute_center.x, absolute_center.y));
    left = left == CENTER ? relative_center.x : left;
    top = top == CENTER ? relative_center.y : top;
    if (!fullscreen)
    {
      windowed_left = left;
      windowed_top = top;
    }
    auto absolute{relative_to_absolute(left, top)};
    if (!SDL_SetWindowPosition(instance, absolute.x, absolute.y))
      throw sdl_exception("Could not set window position to {}, {}", left, top);
    if (auto new_display = SDL_GetDisplayForWindow(instance); display != new_display)
    {
      display = new_display;
      if (display == SDL_DisplayID{0}) throw sdl_exception("Could not get window display index");
      auto relative{absolute_to_relative(absolute.x, absolute.y)};
      left = relative.x;
      top = relative.y;
      if (!fullscreen)
      {
        windowed_left = left;
        windowed_top = top;
      }
    }
  }

  void active::handle_manual_resize(SDL_GPUDevice *device)
  {
    if (!fullscreen)
    {
      windowed_width = width;
      windowed_height = height;
    }
    if (!SDL_SetWindowSize(instance, static_cast<int>(width), static_cast<int>(height)))
      throw sdl_exception("Could not set window size to {}, {}", width, height);
    if (auto new_display = SDL_GetDisplayForWindow(instance); display != new_display)
    {
      display = new_display;
      if (display == SDL_DisplayID{0}) throw sdl_exception("Could not get window display index");
      glm::ivec2 absolute{};
      if (!SDL_GetWindowPosition(instance, &absolute.x, &absolute.y))
        throw sdl_exception("Could not get window position");
      auto relative{absolute_to_relative(absolute.x, absolute.y)};
      left = relative.x;
      top = relative.y;
      if (!fullscreen)
      {
        windowed_left = left;
        windowed_top = top;
      }
    }
    generate_depth_texture(device);
  }

  void active::handle_fullscreen()
  {
    if (fullscreen)
    {
      SDL_Rect display_bounds{};
      if (!SDL_GetDisplayBounds(display, &display_bounds))
        throw sdl_exception("Could not get bounds for display {}", display);
      if (!SDL_SetWindowBordered(instance, false)) throw sdl_exception("Could not set window borderless");
      if (!SDL_SetWindowSize(instance, display_bounds.w, display_bounds.h))
        throw sdl_exception("Could not set window size to {}, {} on display {}", display_bounds.w, display_bounds.h,
                            display);
      if (auto center{calculate_display_center(static_cast<unsigned int>(display_bounds.w),
                                               static_cast<unsigned int>(display_bounds.h))};
          !SDL_SetWindowPosition(instance, center.x, center.y))
        throw sdl_exception("Could not set window position centered on display {}", display);
      return;
    }
    if (!SDL_SetWindowBordered(instance, true)) throw sdl_exception("Could not set window bordered");
    if (!SDL_SetWindowSize(instance, static_cast<int>(windowed_width), static_cast<int>(windowed_height)))
      throw sdl_exception("Could not set window size to {}, {}", windowed_width, windowed_height);
    auto absolute{relative_to_absolute(windowed_left, windowed_top)};
    if (!SDL_SetWindowPosition(instance, absolute.x, absolute.y))
      throw sdl_exception("Could not set window position to {}, {}", absolute.x, absolute.y);
  }

  void active::handle_vsync(SDL_GPUDevice *device)
  {
    if (vsync)
    {
      if (!SDL_SetGPUSwapchainParameters(device, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
        throw sdl_exception("Could not enable VSYNC");
      return;
    }
    if (SDL_WindowSupportsGPUPresentMode(device, instance, SDL_GPU_PRESENTMODE_IMMEDIATE))
      if (!SDL_SetGPUSwapchainParameters(device, instance, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                         SDL_GPU_PRESENTMODE_IMMEDIATE))
        throw sdl_exception("Could not disable VSYNC");
  }

  glm::ivec2 active::calculate_display_center(const unsigned int w, const unsigned int h)
  {
    SDL_Rect bounds{};
    if (!SDL_GetDisplayBounds(display, &bounds)) throw sdl_exception("Could not get bounds for display {}", display);
    return {bounds.x + (bounds.w - static_cast<int>(w)) / 2, bounds.y + (bounds.h - static_cast<int>(h)) / 2};
  }

  glm::ivec2 active::relative_to_absolute(const int x, const int y)
  {
    SDL_Rect bounds{};
    if (!SDL_GetDisplayBounds(display, &bounds)) throw sdl_exception("Could not get bounds for display {}", display);
    return {x + bounds.x, y + bounds.y};
  }

  glm::ivec2 active::absolute_to_relative(const int x, const int y)
  {
    SDL_Rect bounds{};
    if (!SDL_GetDisplayBounds(display, &bounds)) throw sdl_exception("Could not get bounds for display {}", display);
    return {x - bounds.x, y - bounds.y};
  }
}

namespace cse
{
  window::window(const initial &initial_)
    : previous{initial_.title,  initial_.display,    initial_.left,  initial_.top,  initial_.width,
               initial_.height, initial_.fullscreen, initial_.vsync, initial_.mouse},
      active{initial_.title,  initial_.display,    initial_.left,  initial_.top,  initial_.width,
             initial_.height, initial_.fullscreen, initial_.vsync, initial_.mouse}
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
  void window::create(SDL_GPUDevice *device, const double aspect, const unsigned int resolution)
  {
    if (active.phase != help::phase::PREPARED) throw exception("Window must be prepared before creation");
    active.create(device, aspect, resolution, PRIMARY, CENTER);
    active.phase = help::phase::CREATED;
    on_create();
  }

  void window::on_synchronize() {}
  void window::synchronize()
  {
    if (active.phase != help::phase::CREATED) throw exception("Window must be created before synchronization");
    active.synchronize(previous);
    on_synchronize();
  }

  void window::on_event(const SDL_Event &) {}
  void window::event(SDL_GPUDevice *device)
  {
    if (active.phase != help::phase::CREATED) throw exception("Window must be created before processing events");
    switch (active.event.type)
    {
      case SDL_EVENT_QUIT: active.running = false; break;
      case SDL_EVENT_WINDOW_MOVED: active.handle_move(); break;
      case SDL_EVENT_WINDOW_RESIZED: active.handle_resize(device); break;
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

  void window::pre_render(const double) {}
  bool window::start_render(SDL_GPUDevice *device, const double aspect, const glm::dvec3 &clear, const double alpha)
  {
    if (active.phase != help::phase::CREATED) throw exception("Window must be created before pre-rendering");
    active.reconcile(device, PRIMARY, CENTER);
    if (!active.acquire_swapchain_texture(device)) return false;
    pre_render(alpha);
    active.start_render_pass(aspect, clear);
    return true;
  }

  void window::post_render(const double) {}
  void window::end_render(const double alpha)
  {
    if (active.phase != help::phase::CREATED) throw exception("Window must be created before post-rendering");
    active.end_render_pass();
    post_render(alpha);
  }

  void window::on_destroy() {}
  void window::destroy(SDL_GPUDevice *device)
  {
    if (active.phase != help::phase::CREATED) throw exception("Window must be created before destruction");
    active.event = {};
    active.destroy(device);
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
}
