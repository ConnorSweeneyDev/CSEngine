#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "glm/ext/vector_double2.hpp"

#include "core.hpp"
#include "name.hpp"
#include "numeric.hpp"

namespace cse
{
  struct hitbox
  {
    bool operator==(const hitbox &other) const { return name == other.name; }
    cse::name name{};
    double left{};
    double top{};
    double right{};
    double bottom{};
  };

  struct contact
  {
    struct self
    {
      cse::name name{};
      cse::hitbox hitbox{};
    } self;
    struct target
    {
      object *pointer{};
      cse::hitbox hitbox{};
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
    };

    bool overlaps(const rectangle &first, const rectangle &second);
    bool overlaps(const cse::hitbox &first, const cse::hitbox &second);
    std::span<const cse::hitbox> hitboxes(const cse::object *object);
    cse::hitbox bounds(const cse::object *object, const cse::hitbox &source);
    contact describe(const name self_name, cse::object *target, const cse::hitbox &own, const cse::hitbox &theirs);
    cse::hitbox hit(const cse::interface *interface, const glm::dvec2 &point);
  }
}
