#pragma once

#include <memory>
#include <span>
#include <utility>

#include "glm/ext/vector_double2.hpp"

#include "core.hpp"
#include "name.hpp"
#include "numeric.hpp"

namespace cse
{
  using hitbox = name;

  struct contact
  {
    struct self
    {
      class name name{};
      cse::hitbox hitbox{};
      rectangle bounds{};
    } self;
    struct target
    {
      class name name{};
      cse::hitbox hitbox{};
      rectangle bounds{};
    } target;

    axis minimum_axis{};
    glm::dvec2 overlap{};
    glm::dvec2 normal{};
    glm::dvec2 penetration{};
  };

  namespace help
  {
    bool overlaps(const rectangle &first, const rectangle &second);
    std::span<const std::pair<hitbox, rectangle>> current_hitboxes(const std::shared_ptr<object> &object);
    rectangle world_bounds(const std::shared_ptr<object> &object, const rectangle &bounds);
    contact describe_collision(const name self, const name target, const cse::hitbox own, const cse::hitbox theirs,
                               const rectangle &self_bounds, const rectangle &target_bounds);
  }
}
