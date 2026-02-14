#include "collision.hpp"

#include <cmath>
#include <memory>
#include <span>
#include <unordered_map>
#include <utility>

#include "name.hpp"
#include "object.hpp"
#include "resource.hpp"

namespace
{
  std::span<const std::pair<cse::name, cse::hitbox>> current_hitboxes(const std::shared_ptr<cse::object> &object)
  {
    const auto &animation{object->graphics.active.texture.animation};
    auto frame{object->graphics.active.texture.playback.frame};
    if (frame >= animation.frames.size()) return {};
    return animation.frames[frame].hitboxes;
  }

  cse::hitbox world_bounds(const std::shared_ptr<cse::object> &object, const cse::hitbox &hitbox)
  {
    auto width{static_cast<double>(object->graphics.active.texture.image->frame_width)};
    auto height{static_cast<double>(object->graphics.active.texture.image->frame_height)};
    auto position{object->state.active.translation.value};
    auto scale{object->state.active.scale.value};
    auto flip{object->graphics.active.texture.flip};

    double center_x{width / 2.0};
    double center_y{height / 2.0};
    double local_left{hitbox.left - 1.0 - center_x};
    double local_right{hitbox.right - center_x};
    double local_top{center_y - (hitbox.top - 1.0)};
    double local_bottom{center_y - hitbox.bottom};
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

    double scale_x{std::floor(scale.x)};
    double scale_y{std::floor(scale.y)};
    double pixel_x{std::floor(position.x + 0.5) - (static_cast<unsigned int>(width) % 2 == 1 ? 0.5 : 0.0)};
    double pixel_y{std::floor(position.y + 0.5) - (static_cast<unsigned int>(height) % 2 == 1 ? 0.5 : 0.0)};
    return {std::floor(pixel_x + local_left * scale_x + 0.5), std::floor(pixel_y + local_top * scale_y + 0.5),
            std::floor(pixel_x + local_right * scale_x + 0.5), std::floor(pixel_y + local_bottom * scale_y + 0.5)};
  }

  bool overlaps(const cse::hitbox &left, const cse::hitbox &right)
  {
    return left.left < right.right && left.right > right.left && left.bottom < right.top && left.top > right.bottom;
  }
}

namespace cse::help
{
  bool collisions::hit(const name target, const name own, const name theirs) const
  {
    if (auto iterator{entries.find(target)}; iterator != entries.end())
      for (const auto &[left, right] : iterator->second)
        if (left == own && right == theirs) return true;
    return false;
  }

  void collisions::update(const name self, const std::unordered_map<name, std::shared_ptr<object>> &objects)
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
          if (overlaps(self_bounds, other_bounds)) entries[target].push_back({self_hitbox, target_hitbox});
        }
      }
    }
  }

  void collisions::clear() { entries.clear(); }
}
