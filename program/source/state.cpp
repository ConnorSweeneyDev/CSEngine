#include "state.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <execution>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "glm/trigonometric.hpp"

#include "collision.hpp"
#include "object.hpp"
#include "scene.hpp"

namespace cse::help
{
  game_state::game_state(const double tick_) : previous{tick_}, active{tick_} {}

  void game_state::update_previous()
  {
    previous.tick = active.tick;
    previous.window = active.window;
    previous.scenes.clear();
    previous.scenes.reserve(active.scenes.size());
    for (const auto &scene : active.scenes) previous.scenes.insert(scene->name);
    previous.scene = active.scene;
    previous.timer = active.timer;
    previous.phase = active.phase;
  }

  window_state::window_state(const glm::uvec2 &dimensions_, const bool fullscreen_, const bool vsync_)
    : previous{dimensions_.x, dimensions_.y, fullscreen_, vsync_},
      active{dimensions_.x, dimensions_.y, fullscreen_, vsync_}
  {
  }

  void window_state::update_previous()
  {
    previous.width = active.width;
    previous.height = active.height;
    previous.fullscreen = active.fullscreen;
    previous.vsync = active.vsync;
    previous.left = active.left;
    previous.top = active.top;
    previous.display_index = active.display_index;
    previous.running = active.running;
    previous.timer = active.timer;
    previous.phase = active.phase;
  }

  void scene_state::update_previous()
  {
    previous.camera = active.camera;
    previous.objects.clear();
    previous.objects.reserve(active.objects.size());
    for (const auto &object : active.objects) previous.objects.insert(object->name);
    previous.contacts.clear();
    previous.contacts.reserve(active.contacts.size());
    for (const auto &contact : active.contacts) previous.contacts.emplace_back(contact);
    previous.timer = active.timer;
    previous.phase = active.phase;
  }

  void scene_state::generate_order(const std::vector<std::shared_ptr<object>> &objects)
  {
    order.clear();
    for (order.reserve(objects.size()); const auto &object : objects) order.emplace_back(object.get());
    std::sort(order.begin(), order.end(),
              [](const object *left, const object *right)
              {
                if (left->state.active.priority != right->state.active.priority)
                  return left->state.active.priority > right->state.active.priority;
                return left->name.string() < right->name.string();
              });
  }

  void scene_state::generate_contacts()
  {
    active.contacts.clear();
    auto &objects{active.objects};
    if (objects.empty()) return;

    collision::collection hitboxes(objects.size());
    std::vector<std::size_t> indices(objects.size());
    for (std::size_t index{}; index < indices.size(); ++index) indices[index] = index;
    std::for_each(std::execution::par, indices.begin(), indices.end(),
                  [&](std::size_t current)
                  {
                    const auto &object{objects[current]};
                    auto object_hitboxes{collision::hitboxes(object.get())};
                    if (object_hitboxes.empty()) return;
                    auto &[index, z, bounds]{hitboxes[current]};
                    index = current;
                    z = static_cast<std::int64_t>(std::floor(object->state.active.translation.value.z + 0.5));
                    bounds.reserve(object_hitboxes.size());
                    for (const auto &[hitbox, rectangle] : object_hitboxes)
                      bounds.emplace_back(hitbox, collision::bounds(object.get(), rectangle));
                  });

    std::unordered_map<std::int64_t, std::vector<double>> z_dimensions{};
    for (const auto &[index, z, bounds] : hitboxes)
    {
      if (bounds.empty()) continue;
      for (const auto &[hitbox, rectangle] : bounds)
      {
        auto width{rectangle.right - rectangle.left};
        auto height{rectangle.top - rectangle.bottom};
        z_dimensions[z].push_back(std::max(width, height));
      }
    }
    std::unordered_map<std::int64_t, double> z_cell_sizes{};
    for (auto &[z, dimensions] : z_dimensions)
    {
      std::sort(dimensions.begin(), dimensions.end());
      auto median{dimensions[dimensions.size() / 2]};
      z_cell_sizes[z] = std::max(collision::cell_size_minimum, median * 2.0);
    }
    collision::grid grid{};
    for (const auto &[index, z, bounds] : hitboxes)
    {
      if (bounds.empty()) continue;
      const auto cell_size{z_cell_sizes[z]};
      for (const auto &[hitbox, rectangle] : bounds)
      {
        auto min_x{static_cast<std::int64_t>(std::floor(rectangle.left / cell_size))};
        auto max_x{static_cast<std::int64_t>(std::floor(rectangle.right / cell_size))};
        auto min_y{static_cast<std::int64_t>(std::floor(rectangle.bottom / cell_size))};
        auto max_y{static_cast<std::int64_t>(std::floor(rectangle.top / cell_size))};
        for (auto cx{min_x}; cx <= max_x; ++cx)
          for (auto cy{min_y}; cy <= max_y; ++cy) grid[{cx, cy, z}].push_back({index, hitbox, rectangle});
      }
    }

    collision::seen pairs{};
    for (auto &[cell, entries] : grid)
    {
      for (std::size_t first{}; first < entries.size(); ++first)
      {
        const auto &a{entries[first]};
        for (std::size_t second{first + 1}; second < entries.size(); ++second)
        {
          const auto &b{entries[second]};
          if (a.index == b.index) continue;
          if (!collision::overlaps(a.bounds, b.bounds)) continue;
          auto low{std::min(a.index, b.index)}, high{std::max(a.index, b.index)};
          auto low_hitbox{low == a.index ? a.hitbox.identifier() : b.hitbox.identifier()},
            high_hitbox{low == a.index ? b.hitbox.identifier() : a.hitbox.identifier()};
          if (!pairs.emplace(low, low_hitbox, high, high_hitbox).second) continue;
          active.contacts.push_back(
            collision::describe(objects[a.index]->name, objects[b.index], a.hitbox, b.hitbox, a.bounds, b.bounds));
          active.contacts.push_back(
            collision::describe(objects[b.index]->name, objects[a.index], b.hitbox, a.hitbox, b.bounds, a.bounds));
        }
      }
    }
  }

  camera_state::camera_state(const std::tuple<glm::dvec3, glm::dvec3, glm::dvec3> &transform_)
    : previous{std::get<0>(transform_), std::get<1>(transform_), std::get<2>(transform_)},
      active{std::get<0>(transform_), std::get<1>(transform_), std::get<2>(transform_)}
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

  object_state::object_state(const std::tuple<glm::dvec3, double, glm::dvec2> &transform_, const bool collidable_,
                             const int priority_)
    : previous{std::get<0>(transform_), std::get<1>(transform_), std::get<2>(transform_), collidable_, priority_},
      active{std::get<0>(transform_), std::get<1>(transform_), std::get<2>(transform_), collidable_, priority_}
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
}
