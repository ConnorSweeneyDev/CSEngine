#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

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
    struct pair
    {
      struct hash
      {
        std::size_t operator()(const pair &key) const;
      };

      bool operator==(const pair &other) const = default;

      std::size_t a_index;
      std::uint64_t a_hitbox;
      std::size_t b_index;
      std::uint64_t b_hitbox;
    };
    struct cell
    {
      struct hash
      {
        std::size_t operator()(const cell &key) const;
      };

      bool operator==(const cell &other) const = default;

      std::int64_t x{};
      std::int64_t y{};
      std::int64_t z{};
    };
    struct entry
    {
      std::size_t index{};
      cse::hitbox hitbox{};
      rectangle bounds{};
    };
    using collection = std::vector<std::tuple<std::size_t, std::int64_t, std::vector<std::pair<hitbox, rectangle>>>>;
    using grid = std::unordered_map<cell, std::vector<entry>, cell::hash>;
    using cells = std::vector<std::pair<cell, std::vector<entry>>>;
    using seen = std::unordered_set<pair, pair::hash>;

    bool overlaps(const rectangle &first, const rectangle &second);
    std::span<const std::pair<hitbox, rectangle>> hitboxes(const object *object);
    rectangle bounds(const object *object, const rectangle &bounds);
    contact describe(const name self_name, object *target, const hitbox own, const hitbox theirs,
                     const rectangle &self_bounds, const rectangle &target_bounds);

    inline constexpr double cell_size_minimum{16.0};
  }
}
