#include "state.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/trigonometric.hpp"

#include "collision.hpp"
#include "interface.hpp"
#include "object.hpp"
#include "window.hpp"

namespace cse::help
{
  game_state::game_state(const double tick_) : previous{{tick_}}, active{{tick_}} {}

  void game_state::update_previous()
  {
    previous.tick.target = active.tick.target;
    previous.tick.count = active.tick.count;
    previous.tick.average = active.tick.average;
    previous.window = active.window;
    previous.scenes = active.scenes;
    previous.scene = active.scene;
    previous.interfaces = active.interfaces;
    previous.timer = active.timer;
    previous.mixer = active.mixer;
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

  void game_state::reset_targets()
  {
    for (auto *interface : pool) interface->state.active.target.released = {};
  }

  bool game_state::inside(const glm::dvec2 &position, const double aspect, const unsigned int resolution)
  {
    const auto canvas_height{static_cast<double>(std::max(1u, resolution))};
    const auto canvas_width{canvas_height * aspect};
    const auto x{position.x - (std::llround(canvas_width) % 2 == 0 ? 0.5 : 0.0)};
    const auto y{position.y - (std::llround(canvas_height) % 2 == 0 ? 0.5 : 0.0)};
    return x >= -canvas_width / 2.0 && x < canvas_width / 2.0 && y >= -canvas_height / 2.0 && y < canvas_height / 2.0;
  }

  void game_state::interact(const SDL_Event &event, const double aspect, const unsigned int resolution)
  {
    const auto &mouse{active.window->state.active.mouse};
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
      if (event.button.button > SDL_BUTTON_X2 || !inside(mouse.position, aspect, resolution)) return;
      for (auto *interface : pool)
        if (const auto target{collision::hit(interface, mouse.position)}; target != hitbox{})
        {
          interface->state.active.target.pressed[event.button.button] = target;
          break;
        }
    }
    else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
      if (event.button.button > SDL_BUTTON_X2) return;
      for (auto *interface : pool)
        if (const auto target{interface->state.active.target.pressed[event.button.button]}; target != hitbox{})
        {
          interface->state.active.target.released[event.button.button] = target;
          interface->state.active.target.pressed[event.button.button] = {};
        }
    }
  }

  void game_state::hover(SDL_Window *instance, const double aspect, const unsigned int resolution)
  {
    std::optional<glm::dvec2> position{};
    const auto &mouse{active.window->state.active.mouse};
    if (SDL_GetMouseFocus() == instance && inside(mouse.position, aspect, resolution)) position = mouse.position;
    interface *hit{};
    hitbox target{};
    if (position)
      for (auto *interface : pool)
        if (target = collision::hit(interface, *position); target != hitbox{})
        {
          hit = interface;
          break;
        }
    for (auto *interface : pool) interface->state.active.target.hovered = interface == hit ? target : hitbox{};
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
    previous.keyboard = active.keyboard;
    previous.mouse = active.mouse;
    previous.timer = active.timer;
    previous.mixer = active.mixer;
    previous.phase = active.phase;
  }

  void window_state::poll_mouse(SDL_Window *instance, const double aspect, const unsigned int resolution)
  {
    float x{}, y{};
    const auto buttons{SDL_GetMouseState(&x, &y)};
    for (std::size_t button{SDL_BUTTON_LEFT}; button <= SDL_BUTTON_X2; ++button)
      active.mouse.buttons[button] = (buttons & SDL_BUTTON_MASK(button)) != 0;
    if (active.mouse.position != shadow.mouse.position)
    {
      const auto pixel{
        to_pixel(active.mouse.position.x, active.mouse.position.y, active.width, active.height, aspect, resolution)};
      SDL_WarpMouseInWindow(instance, static_cast<float>(pixel.x), static_cast<float>(pixel.y));
    }
    else
      active.mouse.position = to_virtual(x, y, active.width, active.height, aspect, resolution);
    active.mouse.wheel = {};
    shadow.mouse.position = active.mouse.position;
  }

  void window_state::poll_keyboard()
  { std::copy_n(SDL_GetKeyboardState(nullptr), active.keyboard.size(), active.keyboard.begin()); }

  window_state::viewport window_state::letterbox(const unsigned int width, const unsigned int height,
                                                 const double aspect)
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

  glm::dvec2 window_state::to_virtual(const double x, const double y, const unsigned int width,
                                      const unsigned int height, const double aspect, const unsigned int resolution)
  {
    const auto view{letterbox(width, height, aspect)};
    const auto canvas_height{static_cast<double>(std::max(1u, resolution))};
    const auto canvas_width{canvas_height * aspect};
    const glm::dvec2 canvas{(x - view.left) / view.width * canvas_width - canvas_width / 2.0,
                            (y - view.top) / view.height * canvas_height - canvas_height / 2.0};
    return {canvas.x + (std::llround(canvas_width) % 2 == 0 ? 0.5 : 0.0),
            canvas.y + (std::llround(canvas_height) % 2 == 0 ? 0.5 : 0.0)};
  }

  glm::dvec2 window_state::to_pixel(const double x, const double y, const unsigned int width, const unsigned int height,
                                    const double aspect, const unsigned int resolution)
  {
    const auto view{letterbox(width, height, aspect)};
    const auto canvas_height{static_cast<double>(std::max(1u, resolution))};
    const auto canvas_width{canvas_height * aspect};
    const glm::dvec2 canvas{x - (std::llround(canvas_width) % 2 == 0 ? 0.5 : 0.0),
                            y - (std::llround(canvas_height) % 2 == 0 ? 0.5 : 0.0)};
    return {(canvas.x + canvas_width / 2.0) / canvas_width * view.width + view.left,
            (canvas.y + canvas_height / 2.0) / canvas_height * view.height + view.top};
  }

  void scene_state::update_previous()
  {
    previous.camera = active.camera;
    previous.objects = active.objects;
    previous.interfaces = active.interfaces;
    previous.contacts = active.contacts;
    previous.timer = active.timer;
    previous.mixer = active.mixer;
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
    previous.mixer = active.mixer;
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
    previous.mixer = active.mixer;
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
                                   const bool interactable_, const int priority_)
    : previous{translation_, rotation_, scale_, interactable_, priority_},
      active{translation_, rotation_, scale_, interactable_, priority_}
  {
  }

  void interface_state::update_previous()
  {
    previous.translation = active.translation;
    previous.rotation = active.rotation;
    previous.scale = active.scale;
    previous.interactable = active.interactable;
    previous.target.hovered = active.target.hovered;
    previous.target.pressed = active.target.pressed;
    previous.priority = active.priority;
    previous.timer = active.timer;
    previous.mixer = active.mixer;
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
