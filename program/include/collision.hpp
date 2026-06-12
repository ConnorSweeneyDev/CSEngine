#pragma once

#include <cstddef>
#include <cstdint>
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
      cse::name name{};
      cse::hitbox hitbox{};
      rectangle bounds{};
    } self;
    struct target
    {
      object *pointer{};
      cse::hitbox hitbox{};
      rectangle bounds{};
    } target;

    cse::axis axis{};
    glm::dvec2 overlap{};
    glm::dvec2 normal{};
    glm::dvec2 penetration{};
  };

  namespace help::collision
  {
    struct entry
    {
      std::size_t index{};
      std::int64_t z{};
      cse::hitbox hitbox{};
      rectangle bounds{};
    };

    bool overlaps(const rectangle &first, const rectangle &second);
    std::span<const std::pair<hitbox, rectangle>> hitboxes(const object *object);
    rectangle bounds(const object *object, const rectangle &bounds);
    contact describe(const name self_name, object *target, const hitbox own, const hitbox theirs,
                     const rectangle &self_bounds, const rectangle &target_bounds);
    hitbox hit(const interface *interface, const glm::dvec2 &point);
  }
}
