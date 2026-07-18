#include "collision.hpp"

#include <algorithm>
#include <cmath>
#include <span>
#include <utility>

#include "glm/ext/vector_double2.hpp"
#include "glm/trigonometric.hpp"

#include "interface.hpp"
#include "name.hpp"
#include "numeric.hpp"
#include "object.hpp"
#include "resource.hpp"

namespace cse::help::collision
{
  bool overlaps(const rectangle &first, const rectangle &second)
  {
    return first.left < second.right && first.right > second.left && first.bottom < second.top &&
           first.top > second.bottom;
  }

  bool overlaps(const cse::hitbox &first, const cse::hitbox &second)
  {
    return first.left < second.right && first.right > second.left && first.bottom < second.top &&
           first.top > second.bottom;
  }

  std::span<const cse::hitbox> hitboxes(const cse::object *object)
  {
    if (!object->active.collidable) return {};
    const auto &animation{object->active.texture.animation};
    auto frame{object->active.texture.playback.frame};
    if (frame >= animation.frames.size()) return {};
    return animation.frames[frame].hitboxes;
  }

  cse::hitbox bounds(const cse::object *object, const cse::hitbox &source)
  {
    auto width{static_cast<double>(object->active.texture.image.frame_width)};
    auto height{static_cast<double>(object->active.texture.image.frame_height)};
    auto translation{object->active.translation.value};
    auto rotation{static_cast<int>(std::floor(object->active.rotation.value + 0.5))};
    auto scale{object->active.scale.value};
    auto flip{object->active.texture.flip};
    const auto &frames{object->active.texture.animation.frames};
    glm::dvec2 pivot{(width - 1.0) / 2.0, (height - 1.0) / 2.0};
    if (!frames.empty())
    {
      const auto frame{object->active.texture.playback.frame};
      pivot = frames[frame < frames.size() ? frame : frames.size() - 1].pivot;
    }
    const glm::dvec2 anchor{pivot.x + 0.5, pivot.y + 0.5};
    double local_left{source.left - anchor.x};
    double local_right{source.right - anchor.x};
    double local_top{source.top - anchor.y};
    double local_bottom{source.bottom - anchor.y};

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
    const int steps{((rotation % 4) + 4) % 4};
    if (steps != 0)
    {
      const double left{local_left}, right{local_right}, top{local_top}, bottom{local_bottom};
      switch (steps)
      {
        case 1:
          local_left = bottom;
          local_right = top;
          local_bottom = -right;
          local_top = -left;
          break;
        case 2:
          local_left = -right;
          local_right = -left;
          local_bottom = -top;
          local_top = -bottom;
          break;
        case 3:
          local_left = -top;
          local_right = -bottom;
          local_bottom = left;
          local_top = right;
          break;
        default: break;
      }
      if (local_left > local_right) std::swap(local_left, local_right);
      if (local_bottom > local_top) std::swap(local_bottom, local_top);
    }

    glm::dvec2 actual_scale{std::floor(scale.x + 0.5), std::floor(scale.y + 0.5)};
    const bool rotated{steps % 2 == 1};
    if (rotated) std::swap(actual_scale.x, actual_scale.y);
    const glm::dvec2 pixel{std::floor(translation.x + 0.5) - (actual_scale.x / 2.0),
                           std::floor(translation.y + 0.5) + (actual_scale.y / 2.0)};
    return {source.name, std::floor(pixel.x + (local_left * actual_scale.x) + 0.5),
            std::floor(pixel.y + (local_top * actual_scale.y) + 0.5),
            std::floor(pixel.x + (local_right * actual_scale.x) + 0.5),
            std::floor(pixel.y + (local_bottom * actual_scale.y) + 0.5)};
  }

  contact describe(const name self_name, cse::object *target, const cse::hitbox &own, const cse::hitbox &theirs)
  {
    const glm::dvec2 overlap{std::min(own.right, theirs.right) - std::max(own.left, theirs.left),
                             std::min(own.top, theirs.top) - std::max(own.bottom, theirs.bottom)};
    const glm::dvec2 self_center{(own.left + own.right) * 0.5, (own.top + own.bottom) * 0.5};
    const glm::dvec2 target_center{(theirs.left + theirs.right) * 0.5, (theirs.top + theirs.bottom) * 0.5};
    const glm::dvec2 center_delta{target_center.x - self_center.x, target_center.y - self_center.y};

    constexpr double epsilon{1e-9};
    auto axis{axis::NONE};
    if (overlap.x + epsilon < overlap.y)
      axis = axis::X;
    else if (overlap.y + epsilon < overlap.x)
      axis = axis::Y;
    else if (std::abs(center_delta.x) >= std::abs(center_delta.y))
      axis = axis::X;
    else
      axis = axis::Y;
    glm::dvec2 normal{};
    glm::dvec2 penetration{};
    if (axis == axis::X)
    {
      normal.x = center_delta.x >= 0.0 ? 1.0 : -1.0;
      penetration.x = normal.x * overlap.x;
    }
    else if (axis == axis::Y)
    {
      normal.y = center_delta.y >= 0.0 ? 1.0 : -1.0;
      penetration.y = normal.y * overlap.y;
    }

    return {.self = {self_name, own},
            .target = {target, theirs},
            .axis = axis,
            .overlap = overlap,
            .normal = normal,
            .penetration = penetration};
  }

  cse::hitbox hit(const cse::interface *interface, const glm::dvec2 &point)
  {
    if (!interface->active.interactable) return {};
    const auto &animation{interface->active.texture.animation};
    const auto frame{interface->active.texture.playback.frame};
    if (frame >= animation.frames.size()) return {};
    const auto &hitboxes{animation.frames[frame].hitboxes};
    if (hitboxes.empty()) return {};
    const auto scale_x{std::floor(interface->active.scale.value.x + 0.5)};
    const auto scale_y{std::floor(interface->active.scale.value.y + 0.5)};
    if (scale_x <= 0.0 || scale_y <= 0.0) return {};

    const auto &translation{interface->active.translation.value};
    const auto rotation{glm::radians(std::floor(interface->active.rotation.value + 0.5) * -90.0)};
    const auto &pivot{animation.frames[frame].pivot};
    const int steps{((static_cast<int>(std::floor(interface->active.rotation.value + 0.5)) % 4) + 4) % 4};
    const auto extent_x{steps % 2 != 0 ? scale_y : scale_x};
    const auto extent_y{steps % 2 != 0 ? scale_x : scale_y};
    glm::dvec2 local{point.x - std::floor(translation.x + 0.5), point.y - std::floor(translation.y + 0.5)};
    local -= glm::dvec2{std::llround(extent_x) % 2 == 0 ? 0.5 : 0.0, std::llround(extent_y) % 2 == 0 ? -0.5 : 0.0};
    const auto sine{std::sin(-rotation)};
    const auto cosine{std::cos(-rotation)};
    local = {(local.x * cosine) - (local.y * sine), (local.x * sine) + (local.y * cosine)};
    local /= glm::dvec2{scale_x, scale_y};
    const glm::dvec2 anchor{pivot.x + 0.5, pivot.y + 0.5};
    const auto &flip{interface->active.texture.flip};
    for (const auto &entry : hitboxes)
    {
      auto left{entry.left - anchor.x};
      auto right{entry.right - anchor.x};
      auto top{entry.top - anchor.y};
      auto bottom{entry.bottom - anchor.y};
      if (flip.horizontal)
      {
        auto temporary{-left};
        left = -right;
        right = temporary;
      }
      if (flip.vertical)
      {
        auto temporary{-top};
        top = -bottom;
        bottom = temporary;
      }
      if (local.x >= left && local.x < right && local.y <= top && local.y > bottom) return entry;
    }
    return {};
  }
}
