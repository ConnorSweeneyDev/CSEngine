#include "collision.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <span>
#include <utility>

#include "glm/ext/vector_double2.hpp"

#include "name.hpp"
#include "numeric.hpp"
#include "object.hpp"
#include "resource.hpp"

namespace cse::help
{
  bool overlaps(const rectangle &first, const rectangle &second)
  {
    return first.left < second.right && first.right > second.left && first.bottom < second.top &&
           first.top > second.bottom;
  }

  std::span<const std::pair<hitbox, rectangle>> current_hitboxes(const std::shared_ptr<object> &object)
  {
    if (!object->state.active.collidable) return {};
    const auto &animation{object->graphics.active.texture.animation};
    auto frame{object->graphics.active.texture.playback.frame};
    if (frame >= animation.frames.size()) return {};
    return animation.frames[frame].hitboxes;
  }

  rectangle world_bounds(const std::shared_ptr<object> &object, const rectangle &bounds)
  {
    auto width{static_cast<double>(object->graphics.active.texture.image->frame_width)};
    auto height{static_cast<double>(object->graphics.active.texture.image->frame_height)};
    auto position{object->state.active.translation.value};
    auto rotation{static_cast<int>(std::floor(object->state.active.rotation.value + 0.5))};
    auto scale{object->state.active.scale.value};
    auto flip{object->graphics.active.texture.flip};
    glm::dvec2 center{width / 2.0, height / 2.0};
    double local_left{bounds.left - 1.0 - center.x};
    double local_right{bounds.right - center.x};
    double local_top{center.y - (bounds.top - 1.0)};
    double local_bottom{center.y - bounds.bottom};

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
    int steps{((rotation % 4) + 4) % 4};
    if (steps != 0)
    {
      double left{local_left}, right{local_right}, top{local_top}, bottom{local_bottom};
      switch (steps)
      {
        case 1:
          local_left = -top;
          local_right = -bottom;
          local_bottom = left;
          local_top = right;
          break;
        case 2:
          local_left = -right;
          local_right = -left;
          local_bottom = -top;
          local_top = -bottom;
          break;
        case 3:
          local_left = bottom;
          local_right = top;
          local_bottom = -right;
          local_top = -left;
          break;
      }
      if (local_left > local_right) std::swap(local_left, local_right);
      if (local_bottom > local_top) std::swap(local_bottom, local_top);
    }

    glm::dvec2 actual_scale{std::floor(scale.x + 0.5), std::floor(scale.y + 0.5)};
    glm::dvec2 pixel{std::floor(position.x + 0.5) - (static_cast<unsigned int>(width) % 2 == 1 ? 0.5 : 0.0),
                     std::floor(position.y + 0.5) - (static_cast<unsigned int>(height) % 2 == 1 ? 0.5 : 0.0)};
    return {std::floor(pixel.x + local_left * actual_scale.x + 0.5),
            std::floor(pixel.y + local_top * actual_scale.y + 0.5),
            std::floor(pixel.x + local_right * actual_scale.x + 0.5),
            std::floor(pixel.y + local_bottom * actual_scale.y + 0.5)};
  }

  contact describe_collision(const name self, const name target, const hitbox own, const hitbox theirs,
                             const rectangle &self_bounds, const rectangle &target_bounds)
  {
    glm::dvec2 overlap{
      std::min(self_bounds.right, target_bounds.right) - std::max(self_bounds.left, target_bounds.left),
      std::min(self_bounds.top, target_bounds.top) - std::max(self_bounds.bottom, target_bounds.bottom)};
    glm::dvec2 self_center{(self_bounds.left + self_bounds.right) * 0.5, (self_bounds.top + self_bounds.bottom) * 0.5};
    glm::dvec2 target_center{(target_bounds.left + target_bounds.right) * 0.5,
                             (target_bounds.top + target_bounds.bottom) * 0.5};
    glm::dvec2 center_delta{target_center.x - self_center.x, target_center.y - self_center.y};

    constexpr double epsilon{1e-9};
    axis minimum_axis{};
    if (overlap.x + epsilon < overlap.y)
      minimum_axis = axis::X;
    else if (overlap.y + epsilon < overlap.x)
      minimum_axis = axis::Y;
    else if (std::abs(center_delta.x) >= std::abs(center_delta.y))
      minimum_axis = axis::X;
    else
      minimum_axis = axis::Y;
    glm::dvec2 normal{};
    glm::dvec2 penetration{};
    if (minimum_axis == axis::X)
    {
      normal.x = center_delta.x >= 0.0 ? 1.0 : -1.0;
      penetration.x = normal.x * overlap.x;
    }
    else if (minimum_axis == axis::Y)
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
