#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "glm/ext/vector_double2.hpp"

#include "core.hpp"
#include "group.hpp"
#include "name.hpp"
#include "numeric.hpp"
#include "resource.hpp"

namespace cse
{
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
      std::int32_t left{};
      std::int32_t bottom{};
      std::int32_t right{};
      std::int32_t top{};
      std::int32_t z{};
      std::uint32_t object{};
      cse::group self{};
      cse::group target{};
    };
    struct slot
    {
      std::uint32_t entry{};
      std::int32_t x{};
      std::int32_t y{};
      std::int32_t bit{};
    };

    std::int32_t quantize(const double value);
    std::size_t cell(const std::int32_t z, const std::int32_t x, const std::int32_t y, const int bit);
    bool overlaps(const rectangle &first, const rectangle &second);
    bool overlaps(const cse::hitbox &first, const cse::hitbox &second);
    std::span<const cse::hitbox> hitboxes(const cse::object *object);
    cse::hitbox bounds(const cse::object *object, const cse::hitbox &source);
    contact describe(const name self_name, cse::object *target, const cse::hitbox &own, const cse::hitbox &theirs);
    contact mirror(const contact &source, const name self_name, cse::object *target);
    cse::hitbox hit(const cse::interface *interface, const glm::dvec2 &point);
  }
}
