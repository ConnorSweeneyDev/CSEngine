#include "collisions.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <span>
#include <unordered_map>
#include <utility>

#include "glm/ext/vector_double2.hpp"

#include "hitbox.hpp"
#include "name.hpp"
#include "numeric.hpp"
#include "object.hpp"
#include "resource.hpp"

namespace
{
  std::span<const std::pair<cse::hitbox, cse::rectangle>> current_hitboxes(const std::shared_ptr<cse::object> &object)
  {
    const auto &animation{object->graphics.active.texture.animation};
    auto frame{object->graphics.active.texture.playback.frame};
    if (frame >= animation.frames.size()) return {};
    return animation.frames[frame].hitboxes;
  }

  cse::rectangle world_bounds(const std::shared_ptr<cse::object> &object, const cse::rectangle &hitbox)
  {
    auto width{static_cast<double>(object->graphics.active.texture.image->frame_width)};
    auto height{static_cast<double>(object->graphics.active.texture.image->frame_height)};
    auto position{object->state.active.translation.value};
    auto scale{object->state.active.scale.value};
    auto flip{object->graphics.active.texture.flip};

    glm::dvec2 center{width / 2.0, height / 2.0};
    double local_left{hitbox.left - 1.0 - center.x};
    double local_right{hitbox.right - center.x};
    double local_top{center.y - (hitbox.top - 1.0)};
    double local_bottom{center.y - hitbox.bottom};
    if (flip.horizontal)
    {
      auto temporary{-local_left};
      local_left = -local_right;
      local_right = temporary;
    }
    if (flip.vertical)
    {
      auto temporary{-local_top};
      local_top = -local_bottom;
      local_bottom = temporary;
    }

    glm::dvec2 floored_scale{std::floor(scale.x), std::floor(scale.y)};
    glm::dvec2 pixel{std::floor(position.x + 0.5) - (static_cast<unsigned int>(width) % 2 == 1 ? 0.5 : 0.0),
                     std::floor(position.y + 0.5) - (static_cast<unsigned int>(height) % 2 == 1 ? 0.5 : 0.0)};
    return {std::floor(pixel.x + local_left * floored_scale.x + 0.5),
            std::floor(pixel.y + local_top * floored_scale.y + 0.5),
            std::floor(pixel.x + local_right * floored_scale.x + 0.5),
            std::floor(pixel.y + local_bottom * floored_scale.y + 0.5)};
  }

  bool overlaps(const cse::rectangle &left, const cse::rectangle &right)
  {
    return left.left < right.right && left.right > right.left && left.bottom < right.top && left.top > right.bottom;
  }

  cse::contact describe_collision(const cse::name self, const cse::name target, const cse::hitbox own,
                                  const cse::hitbox theirs, const cse::rectangle &self_bounds,
                                  const cse::rectangle &target_bounds)
  {
    constexpr double epsilon{1e-9};

    glm::dvec2 overlap{
      std::min(self_bounds.right, target_bounds.right) - std::max(self_bounds.left, target_bounds.left),
      std::min(self_bounds.top, target_bounds.top) - std::max(self_bounds.bottom, target_bounds.bottom)};

    glm::dvec2 self_center{(self_bounds.left + self_bounds.right) * 0.5, (self_bounds.top + self_bounds.bottom) * 0.5};
    glm::dvec2 target_center{(target_bounds.left + target_bounds.right) * 0.5,
                             (target_bounds.top + target_bounds.bottom) * 0.5};
    glm::dvec2 center_delta{target_center.x - self_center.x, target_center.y - self_center.y};

    cse::axis minimum_axis{};
    if (overlap.x + epsilon < overlap.y)
      minimum_axis = cse::axis::X;
    else if (overlap.y + epsilon < overlap.x)
      minimum_axis = cse::axis::Y;
    else if (std::fabs(center_delta.x) >= std::fabs(center_delta.y))
      minimum_axis = cse::axis::X;
    else
      minimum_axis = cse::axis::Y;

    glm::dvec2 normal{};
    glm::dvec2 penetration{};
    if (minimum_axis == cse::axis::X)
    {
      normal.x = center_delta.x >= 0.0 ? 1.0 : -1.0;
      penetration.x = normal.x * overlap.x;
    }
    else if (minimum_axis == cse::axis::Y)
    {
      normal.y = center_delta.y >= 0.0 ? 1.0 : -1.0;
      penetration.y = normal.y * overlap.y;
    }

    return {.self = {self, own, self_bounds},
            .target = {target, theirs, target_bounds},
            .minimum_axis = minimum_axis,
            .overlap = overlap,
            .normal = normal,
            .penetration = penetration};
  }
}

namespace cse::help
{
  void collisions::update(const name self, const std::unordered_map<hitbox, std::shared_ptr<object>> &objects)
  {
    auto iterator{objects.find(self)};
    if (iterator == objects.end()) return;
    auto &self_pointer{iterator->second};
    auto self_hitboxes{current_hitboxes(self_pointer)};
    if (self_hitboxes.empty()) return;

    auto self_z{std::floor(self_pointer->state.active.translation.value.z + 0.5)};
    for (const auto &[target, target_pointer] : objects)
    {
      if (self == target) continue;
      if (std::floor(target_pointer->state.active.translation.value.z + 0.5) != self_z) continue;
      auto target_hitboxes{current_hitboxes(target_pointer)};
      if (target_hitboxes.empty()) continue;
      for (const auto &[self_hitbox, self_hitbox_object] : self_hitboxes)
      {
        auto self_bounds{world_bounds(self_pointer, self_hitbox_object)};
        for (const auto &[target_hitbox, target_hitbox_object] : target_hitboxes)
        {
          auto other_bounds{world_bounds(target_pointer, target_hitbox_object)};
          if (overlaps(self_bounds, other_bounds))
            contacts[target].push_back(
              describe_collision(self, target, self_hitbox, target_hitbox, self_bounds, other_bounds));
        }
      }
    }
  }

  void collisions::clear() { contacts.clear(); }
}
