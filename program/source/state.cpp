#include "state.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/ext/vector_int2.hpp"
#include "glm/trigonometric.hpp"

#include "collision.hpp"
#include "exception.hpp"
#include "interface.hpp"
#include "object.hpp"

namespace cse::help
{
  game_state::game_state(const double tick_) : previous{tick_}, active{tick_} {}

  void game_state::update_previous()
  {
    previous.tick = active.tick;
    previous.window = active.window;
    previous.scenes = active.scenes;
    previous.scene = active.scene;
    previous.interfaces = active.interfaces;
    previous.timer = active.timer;
    previous.phase = active.phase;
  }

  void game_state::generate_order(const std::vector<std::shared_ptr<interface>> &interfaces)
  {
    order.clear();
    for (order.reserve(interfaces.size()); const auto &interface : interfaces) order.emplace_back(interface.get());
    std::sort(order.begin(), order.end(),
              [](const interface *left, const interface *right)
              {
                if (left->state.active.priority != right->state.active.priority)
                  return left->state.active.priority > right->state.active.priority;
                return left->name.identifier() < right->name.identifier();
              });
  }

  void game_state::generate_pool(const std::vector<interface *> &interfaces)
  {
    pool.clear();
    pool.reserve(interfaces.size() + order.size());
    pool.insert(pool.end(), interfaces.begin(), interfaces.end());
    pool.insert(pool.end(), order.begin(), order.end());
    std::sort(pool.begin(), pool.end(),
              [](const interface *left, const interface *right)
              {
                if (left->state.active.priority != right->state.active.priority)
                  return left->state.active.priority > right->state.active.priority;
                if (const auto left_layer{left->scene ? 0 : 1}, right_layer{right->scene ? 0 : 1};
                    left_layer != right_layer)
                  return left_layer > right_layer;
                return left->name.identifier() < right->name.identifier();
              });
  }

  std::optional<glm::dvec2> game_state::locate(const float x, const float y, const unsigned int width,
                                               const unsigned int height, const double aspect,
                                               const unsigned int resolution) const
  {
    const auto window_width{static_cast<double>(width)};
    const auto window_height{static_cast<double>(height)};
    double viewport_left{}, viewport_top{}, viewport_width{}, viewport_height{};
    if (window_width / window_height > aspect)
    {
      viewport_height = window_height;
      viewport_width = viewport_height * aspect;
      viewport_left = (window_width - viewport_width) / 2.0;
    }
    else
    {
      viewport_width = window_width;
      viewport_height = viewport_width / aspect;
      viewport_top = (window_height - viewport_height) / 2.0;
    }
    const auto window_x{static_cast<double>(x)};
    const auto window_y{static_cast<double>(y)};
    if (window_x < viewport_left || window_x >= viewport_left + viewport_width || window_y < viewport_top ||
        window_y >= viewport_top + viewport_height)
      return std::nullopt;
    const auto canvas_height{static_cast<double>(std::max(1u, resolution))};
    const auto canvas_width{canvas_height * aspect};
    const glm::dvec2 canvas{(window_x - viewport_left) / viewport_width * canvas_width - canvas_width / 2.0,
                            (window_y - viewport_top) / viewport_height * canvas_height - canvas_height / 2.0};
    return glm::dvec2{canvas.x + (std::llround(canvas_width) % 2 == 0 ? 0.5 : 0.0),
                      canvas.y + (std::llround(canvas_height) % 2 == 0 ? 0.5 : 0.0)};
  }

  void game_state::interact(const SDL_Event &event, const unsigned int width, const unsigned int height,
                            const double aspect, const unsigned int resolution)
  {
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
    {
      const auto position{locate(event.button.x, event.button.y, width, height, aspect, resolution)};
      if (!position) return;
      for (auto *interface : pool)
        if (const auto target{collision::hit(interface, *position)}; !(target == hitbox{}))
        {
          interface->state.active.pressed = target;
          interface->on_press(target);
          break;
        }
    }
    else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT)
    {
      const auto position{locate(event.button.x, event.button.y, width, height, aspect, resolution)};
      interface *hit{};
      hitbox top{};
      if (position)
        for (auto *interface : pool)
          if (top = collision::hit(interface, *position); !(top == hitbox{}))
          {
            hit = interface;
            break;
          }
      for (auto *interface : pool)
        if (const auto target{interface->state.active.pressed}; !(target == hitbox{}))
        {
          interface->state.active.pressed = {};
          interface->on_release(target);
          if (interface == hit && target == top) interface->on_click(target);
        }
    }
  }

  void game_state::hover(SDL_Window *instance, const unsigned int width, const unsigned int height, const double aspect,
                         const unsigned int resolution)
  {
    std::optional<glm::dvec2> position{};
    if (SDL_GetMouseFocus() == instance)
    {
      float x{}, y{};
      SDL_GetMouseState(&x, &y);
      position = locate(x, y, width, height, aspect, resolution);
    }
    interface *hit{};
    hitbox target{};
    if (position)
      for (auto *interface : pool)
        if (target = collision::hit(interface, *position); !(target == hitbox{}))
        {
          hit = interface;
          break;
        }
    for (auto *interface : pool)
    {
      const auto current{interface == hit ? target : hitbox{}};
      auto &hovered{interface->state.active.hovered};
      if (current == hovered) continue;
      const auto previous_target{hovered};
      hovered = current;
      if (!(previous_target == hitbox{})) interface->on_unhover(previous_target);
      if (!(current == hitbox{})) interface->on_hover(current);
    }
  }

  window_state::window_state(const SDL_DisplayID display_, const int left_, const int top_, const unsigned int width_,
                             const unsigned int height_)
    : previous{display_, left_, top_, width_, height_}, active{display_, left_, top_, width_, height_}
  {
  }

  void window_state::update_previous()
  {
    previous.display = active.display;
    previous.left = active.left;
    previous.top = active.top;
    previous.width = active.width;
    previous.height = active.height;
    previous.running = active.running;
    previous.timer = active.timer;
    previous.phase = active.phase;
  }

  void window_state::handle_move(const bool fullscreen, SDL_Window *instance)
  {
    if (fullscreen) return;
    active.display.value = SDL_GetDisplayForWindow(instance);
    if (active.display == SDL_DisplayID{0}) throw sdl_exception("Could not get window display index");
    glm::ivec2 absolute{};
    if (!SDL_GetWindowPosition(instance, &absolute.x, &absolute.y))
      throw sdl_exception("Could not get window position");
    auto relative{absolute_to_relative(active.display, absolute.x, absolute.y)};
    active.left.value = relative.x;
    active.top.value = relative.y;
    windowed_left = active.left;
    windowed_top = active.top;
  }

  void window_state::handle_manual_move(const bool fullscreen, SDL_Window *instance, const int CENTER)
  {
    auto absolute_center{calculate_display_center(active.display, active.width, active.height)};
    auto relative_center(absolute_to_relative(active.display, absolute_center.x, absolute_center.y));
    active.left.value = active.left == CENTER ? relative_center.x : static_cast<int>(active.left);
    active.top.value = active.top == CENTER ? relative_center.y : static_cast<int>(active.top);
    if (!fullscreen)
    {
      windowed_left = active.left;
      windowed_top = active.top;
    }
    auto absolute{relative_to_absolute(active.display, active.left, active.top)};
    if (!SDL_SetWindowPosition(instance, absolute.x, absolute.y))
      throw sdl_exception("Could not set window position to {}, {}", active.left, active.top);
    if (auto new_display = SDL_GetDisplayForWindow(instance); active.display != new_display)
    {
      active.display.value = new_display;
      if (active.display == SDL_DisplayID{0}) throw sdl_exception("Could not get window display index");
      auto relative{absolute_to_relative(active.display, absolute.x, absolute.y)};
      active.left.value = relative.x;
      active.top.value = relative.y;
      if (!fullscreen)
      {
        windowed_left = active.left;
        windowed_top = active.top;
      }
    }
  }

  void window_state::handle_manual_display_move(const bool fullscreen, SDL_Window *instance,
                                                const SDL_DisplayID PRIMARY)
  {
    if (active.display == PRIMARY) active.display.value = SDL_GetPrimaryDisplay();
    auto absolute_center{calculate_display_center(active.display, active.width, active.height)};
    auto relative_center(absolute_to_relative(active.display, absolute_center.x, absolute_center.y));
    active.left.value = relative_center.x;
    active.top.value = relative_center.y;
    if (!fullscreen)
    {
      windowed_left = active.left;
      windowed_top = active.top;
    }
    auto absolute{relative_to_absolute(active.display, active.left, active.top)};
    if (!SDL_SetWindowPosition(instance, absolute.x, absolute.y))
      throw sdl_exception("Could not set window position centered on display {}", active.display);
  }

  void window_state::handle_resize(
    const bool fullscreen, SDL_Window *instance, SDL_GPUDevice *gpu, SDL_GPUTexture *&depth_texture,
    std::function<void(const unsigned int, const unsigned int, SDL_GPUDevice *, SDL_GPUTexture *&)>
      generate_depth_texture)
  {
    if (auto new_display = SDL_GetDisplayForWindow(instance); active.display != new_display)
    {
      active.display.value = new_display;
      if (active.display == SDL_DisplayID{0}) throw sdl_exception("Could not get window display index");
      glm::ivec2 absolute{};
      if (!SDL_GetWindowPosition(instance, &absolute.x, &absolute.y))
        throw sdl_exception("Could not get window position");
      auto relative{absolute_to_relative(active.display, absolute.x, absolute.y)};
      active.left.value = relative.x;
      active.top.value = relative.y;
      if (!fullscreen)
      {
        windowed_left = active.left;
        windowed_top = active.top;
      }
    }
    int current_width{}, current_height{};
    SDL_GetWindowSize(instance, &current_width, &current_height);
    if (current_width <= 0) current_width = 1;
    if (current_height <= 0) current_height = 1;
    active.width.value = static_cast<unsigned int>(current_width);
    active.height.value = static_cast<unsigned int>(current_height);
    if (!fullscreen)
    {
      windowed_width = active.width;
      windowed_height = active.height;
    }
    generate_depth_texture(active.width, active.height, gpu, depth_texture);
  }

  void window_state::handle_manual_resize(
    const bool fullscreen, SDL_Window *instance, SDL_GPUDevice *gpu, SDL_GPUTexture *&depth_texture,
    std::function<void(const unsigned int, const unsigned int, SDL_GPUDevice *, SDL_GPUTexture *&)>
      generate_depth_texture)
  {
    if (!fullscreen)
    {
      windowed_width = active.width;
      windowed_height = active.height;
    }
    if (!SDL_SetWindowSize(instance, static_cast<int>(active.width), static_cast<int>(active.height)))
      throw sdl_exception("Could not set window size to {}, {}", active.width, active.height);
    if (auto new_display = SDL_GetDisplayForWindow(instance); active.display != new_display)
    {
      active.display.value = new_display;
      if (active.display == SDL_DisplayID{0}) throw sdl_exception("Could not get window display index");
      glm::ivec2 absolute{};
      if (!SDL_GetWindowPosition(instance, &absolute.x, &absolute.y))
        throw sdl_exception("Could not get window position");
      auto relative{absolute_to_relative(active.display, absolute.x, absolute.y)};
      active.left.value = relative.x;
      active.top.value = relative.y;
      if (!fullscreen)
      {
        windowed_left = active.left;
        windowed_top = active.top;
      }
    }
    generate_depth_texture(active.width, active.height, gpu, depth_texture);
  }

  glm::ivec2 window_state::calculate_display_center(const SDL_DisplayID display, const unsigned int width,
                                                    const unsigned int height)
  {
    SDL_Rect bounds{};
    if (!SDL_GetDisplayBounds(display, &bounds)) throw sdl_exception("Could not get bounds for display {}", display);
    return {bounds.x + (bounds.w - static_cast<int>(width)) / 2, bounds.y + (bounds.h - static_cast<int>(height)) / 2};
  }

  glm::ivec2 window_state::relative_to_absolute(const SDL_DisplayID display, const int left, const int top)
  {
    SDL_Rect bounds{};
    if (!SDL_GetDisplayBounds(display, &bounds)) throw sdl_exception("Could not get bounds for display {}", display);
    return {left + bounds.x, top + bounds.y};
  }

  glm::ivec2 window_state::absolute_to_relative(const SDL_DisplayID display, const int left, const int top)
  {
    SDL_Rect bounds{};
    if (!SDL_GetDisplayBounds(display, &bounds)) throw sdl_exception("Could not get bounds for display {}", display);
    return {left - bounds.x, top - bounds.y};
  }

  void scene_state::update_previous()
  {
    previous.camera = active.camera;
    previous.objects = active.objects;
    previous.interfaces = active.interfaces;
    previous.contacts = active.contacts;
    previous.timer = active.timer;
    previous.phase = active.phase;
  }

  void scene_state::generate_order(const std::vector<std::shared_ptr<object>> &objects)
  {
    object_order.clear();
    for (object_order.reserve(objects.size()); const auto &object : objects) object_order.emplace_back(object.get());
    std::sort(object_order.begin(), object_order.end(),
              [](const object *left, const object *right)
              {
                if (left->state.active.priority != right->state.active.priority)
                  return left->state.active.priority > right->state.active.priority;
                return left->name.identifier() < right->name.identifier();
              });
  }

  void scene_state::generate_order(const std::vector<std::shared_ptr<interface>> &interfaces)
  {
    interface_order.clear();
    for (interface_order.reserve(interfaces.size()); const auto &interface : interfaces)
      interface_order.emplace_back(interface.get());
    std::sort(interface_order.begin(), interface_order.end(),
              [](const interface *left, const interface *right)
              {
                if (left->state.active.priority != right->state.active.priority)
                  return left->state.active.priority > right->state.active.priority;
                return left->name.identifier() < right->name.identifier();
              });
  }

  void scene_state::generate_contacts()
  {
    active.contacts.clear();
    auto &objects{active.objects};
    if (objects.empty()) return;

    static std::vector<collision::entry> entries{};
    entries.clear();
    entries.reserve(objects.size() * 4);
    for (std::size_t index{}; index < objects.size(); ++index)
    {
      const auto &object{objects[index]};
      auto object_hitboxes{collision::hitboxes(object.get())};
      if (object_hitboxes.empty()) continue;
      auto z{static_cast<std::int64_t>(std::floor(object->state.active.translation.value.z + 0.5))};
      for (const auto &[hitbox, rectangle] : object_hitboxes)
        entries.push_back({index, z, hitbox, collision::bounds(object.get(), rectangle)});
    }
    if (entries.empty()) return;

    auto comparator{[](const collision::entry &a, const collision::entry &b)
                    {
                      if (a.z != b.z) return a.z < b.z;
                      return a.bounds.left < b.bounds.left;
                    }};
    bool large_disorder{};
    for (std::size_t index{1}; index < entries.size(); ++index)
      if (comparator(entries[index], entries[index - 1]))
      {
        large_disorder = true;
        break;
      }

    if (large_disorder) { std::sort(entries.begin(), entries.end(), comparator); }
    else
      for (std::size_t first{1}; first < entries.size(); ++first)
      {
        auto key{entries[first]};
        std::size_t second{first};
        while (second > 0 && comparator(key, entries[second - 1]))
        {
          entries[second] = entries[second - 1];
          --second;
        }
        entries[second] = key;
      }

    static std::vector<std::size_t> active_list{};
    active_list.clear();
    active_list.reserve(std::min(entries.size(), static_cast<std::size_t>(64)));
    std::size_t start{0};
    while (start < entries.size())
    {
      auto z{entries[start].z};
      std::size_t end{start + 1};
      while (end < entries.size() && entries[end].z == z) ++end;

      active_list.clear();
      for (std::size_t first{start}; first < end; ++first)
      {
        const auto &current{entries[first]};
        for (std::size_t second{}; second < active_list.size();)
        {
          const auto &other{entries[active_list[second]]};
          if (other.bounds.right < current.bounds.left)
          {
            active_list[second] = active_list.back();
            active_list.pop_back();
            continue;
          }
          if (current.index != other.index && collision::overlaps(current.bounds, other.bounds))
          {
            active.contacts.push_back(collision::describe(objects[current.index]->name, objects[other.index].get(),
                                                          current.hitbox, other.hitbox, current.bounds, other.bounds));
            active.contacts.push_back(collision::describe(objects[other.index]->name, objects[current.index].get(),
                                                          other.hitbox, current.hitbox, other.bounds, current.bounds));
          }
          ++second;
        }
        active_list.push_back(first);
      }
      start = end;
    }
  }

  camera_state::camera_state(const glm::dvec3 &translation_, const glm::dvec3 &forward_, const glm::dvec3 &up_)
    : previous{translation_, forward_, up_}, active{translation_, forward_, up_}
  {
  }

  void camera_state::update_previous()
  {
    previous.translation = active.translation;
    previous.forward = active.forward;
    previous.up = active.up;
    previous.timer = active.timer;
    previous.phase = active.phase;
  }

  glm::dmat4 camera_state::calculate_view_matrix(const double alpha) const
  {
    auto translation = previous.translation.value + (active.translation.value - previous.translation.value) * alpha;
    auto forward = previous.forward.value + (active.forward.value - previous.forward.value) * alpha;
    auto up = previous.up.value + (active.up.value - previous.up.value) * alpha;
    return glm::lookAt(translation, translation + forward, up);
  }

  object_state::object_state(const glm::dvec3 &translation_, const double rotation_, const glm::dvec2 &scale_,
                             const bool collidable_, const int priority_)
    : previous{translation_, rotation_, scale_, collidable_, priority_},
      active{translation_, rotation_, scale_, collidable_, priority_}
  {
  }

  void object_state::update_previous()
  {
    previous.translation = active.translation;
    previous.rotation = active.rotation;
    previous.scale = active.scale;
    previous.collidable = active.collidable;
    previous.priority = active.priority;
    previous.timer = active.timer;
    previous.phase = active.phase;
  }

  glm::dmat4 object_state::calculate_model_matrix(const unsigned int frame_width, const unsigned int frame_height,
                                                  const double alpha) const
  {
    auto translation = previous.translation.value + (active.translation.value - previous.translation.value) * alpha;
    auto rotation = previous.rotation.value + (active.rotation.value - previous.rotation.value) * alpha;
    auto scale = previous.scale.value + (active.scale.value - previous.scale.value) * alpha;
    auto model_matrix{glm::dmat4(1.0)};
    model_matrix = glm::translate(model_matrix, {std::floor(translation.x + 0.5) - (frame_width % 2 == 1 ? 0.5 : 0.0),
                                                 std::floor(translation.y + 0.5) - (frame_height % 2 == 1 ? 0.5 : 0.0),
                                                 std::floor(translation.z + 0.5)});
    model_matrix = glm::rotate(model_matrix, 0.0, {1.0, 0.0, 0.0});
    model_matrix = glm::rotate(model_matrix, 0.0, {0.0, 1.0, 0.0});
    model_matrix = glm::rotate(model_matrix, glm::radians(std::floor(rotation + 0.5) * 90.0), {0.0, 0.0, 1.0});
    model_matrix = glm::scale(model_matrix, {std::floor(scale.x + 0.5) * static_cast<double>(frame_width) / 2.0,
                                             std::floor(scale.y + 0.5) * static_cast<double>(frame_height) / 2.0, 1.0});
    return model_matrix;
  }

  interface_state::interface_state(const glm::dvec2 &translation_, const double rotation_, const glm::dvec2 &scale_,
                                   const std::string &text_, const int priority_)
    : previous{
        .translation = translation_, .rotation = rotation_, .scale = scale_, .text = text_, .priority = priority_},
      active{.translation = translation_, .rotation = rotation_, .scale = scale_, .text = text_, .priority = priority_}
  {
  }

  void interface_state::update_previous()
  {
    previous.translation = active.translation;
    previous.rotation = active.rotation;
    previous.scale = active.scale;
    previous.text = active.text;
    previous.hovered = active.hovered;
    previous.pressed = active.pressed;
    previous.priority = active.priority;
    previous.timer = active.timer;
    previous.phase = active.phase;
  }

  glm::dmat4 interface_state::calculate_model_matrix(const unsigned int frame_width, const unsigned int frame_height,
                                                     const double alpha) const
  {
    auto translation = previous.translation.value + (active.translation.value - previous.translation.value) * alpha;
    auto rotation = previous.rotation.value + (active.rotation.value - previous.rotation.value) * alpha;
    auto scale = previous.scale.value + (active.scale.value - previous.scale.value) * alpha;
    auto model_matrix{glm::dmat4(1.0)};
    model_matrix =
      glm::translate(model_matrix, {std::floor(translation.x + 0.5), std::floor(translation.y + 0.5), 0.0});
    model_matrix = glm::rotate(model_matrix, glm::radians(std::floor(rotation + 0.5) * -90.0), {0.0, 0.0, 1.0});
    model_matrix =
      glm::translate(model_matrix, {frame_width % 2 == 0 ? 0.5 : 0.0, frame_height % 2 == 0 ? 0.5 : 0.0, 0.0});
    model_matrix = glm::scale(model_matrix, {std::floor(scale.x + 0.5) * static_cast<double>(frame_width) / 2.0,
                                             std::floor(scale.y + 0.5) * static_cast<double>(frame_height) / 2.0, 1.0});
    return model_matrix;
  }

  glm::dmat4 interface_state::calculate_text_matrix(const unsigned int width, const unsigned int height,
                                                    const double alpha) const
  {
    auto translation = previous.translation.value + (active.translation.value - previous.translation.value) * alpha;
    auto rotation = previous.rotation.value + (active.rotation.value - previous.rotation.value) * alpha;
    auto model_matrix{glm::dmat4(1.0)};
    model_matrix =
      glm::translate(model_matrix, {std::floor(translation.x + 0.5), std::floor(translation.y + 0.5), 0.0});
    model_matrix = glm::rotate(model_matrix, glm::radians(std::floor(rotation + 0.5) * -90.0), {0.0, 0.0, 1.0});
    model_matrix = glm::translate(model_matrix, {width % 2 == 0 ? 0.5 : 0.0, height % 2 == 0 ? 0.5 : 0.0, 0.0});
    model_matrix = glm::scale(model_matrix, {width / 2.0, height / 2.0, 1.0});
    return model_matrix;
  }
}
