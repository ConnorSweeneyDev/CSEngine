#pragma once

#include "collision.hpp"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string_view>

namespace cse
{
  constexpr collider::collider(const std::uint64_t value_) : value{value_} {}

  constexpr collider collider::operator|(const collider &other) const { return collider{value | other.value}; }

  constexpr collider collider::operator&(const collider &other) const { return collider{value & other.value}; }

  constexpr collider collider::operator^(const collider &other) const { return collider{value ^ other.value}; }

  constexpr collider collider::operator~() const { return collider{~value}; }

  constexpr collider &collider::operator|=(const collider &other)
  {
    value |= other.value;
    return *this;
  }

  constexpr collider &collider::operator&=(const collider &other)
  {
    value &= other.value;
    return *this;
  }

  constexpr collider &collider::operator^=(const collider &other)
  {
    value ^= other.value;
    return *this;
  }

  constexpr bool collider::empty() const { return value == 0; }

  constexpr std::uint64_t collider::bits() const { return value; }
}

namespace cse::help::collision
{
  constexpr bool distinct(const std::span<const std::string_view> names)
  {
    for (auto first{names.begin()}; first != names.end(); ++first)
      for (auto second{std::next(first)}; second != names.end(); ++second)
        if (*first == *second) return false;
    return true;
  }

  constexpr std::size_t position(const std::span<const std::string_view> names, const std::string_view label)
  {
    std::size_t index{};
    for (const auto name : names)
    {
      if (name == label) return index;
      ++index;
    }
    throw std::out_of_range{"A collider was requested that no COLLIDERS declaration provides"};
  }

  template <const auto &names> constexpr cse::collider forge(const std::string_view label)
  {
    static_assert(distinct(names), "COLLIDERS declares the same collider more than once");
    static_assert(std::size(names) <= 64, "COLLIDERS declares more than 64 colliders");
    return cse::collider{std::uint64_t{1} << position(names, label)};
  }

  template <const auto &names> constexpr cse::collider every()
  {
    static_assert(std::size(names) <= 64, "COLLIDERS declares more than 64 colliders");
    return cse::collider{std::size(names) == 64 ? ~std::uint64_t{}
                                                : (std::uint64_t{1} << std::size(names)) - std::uint64_t{1}};
  }
}
