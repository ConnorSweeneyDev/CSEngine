#pragma once

#include "group.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string_view>

namespace cse
{
  constexpr group::group(const word &value_) : value{value_} {}

  constexpr group group::operator|(const group &other) const
  {
    auto result{*this};
    return result |= other;
  }

  constexpr group group::operator&(const group &other) const
  {
    auto result{*this};
    return result &= other;
  }

  constexpr group group::operator^(const group &other) const
  {
    auto result{*this};
    return result ^= other;
  }

  constexpr group group::operator~() const
  {
    auto result{*this};
    std::ranges::transform(result.value, result.value.begin(), std::bit_not{});
    return result;
  }

  constexpr group &group::operator|=(const group &other)
  {
    std::ranges::transform(value, other.value, value.begin(), std::bit_or{});
    return *this;
  }

  constexpr group &group::operator&=(const group &other)
  {
    std::ranges::transform(value, other.value, value.begin(), std::bit_and{});
    return *this;
  }

  constexpr group &group::operator^=(const group &other)
  {
    std::ranges::transform(value, other.value, value.begin(), std::bit_xor{});
    return *this;
  }

  constexpr bool group::empty() const { return value == word{}; }

  constexpr const group::word &group::words() const { return value; }

  inline constexpr group nothing{};
  inline constexpr group everything{~nothing};
}

namespace cse::help::group
{
  constexpr bool distinct(const std::span<const std::string_view> names)
  {
    for (auto first{names.begin()}; first != names.end(); ++first)
      for (auto second{std::next(first)}; second != names.end(); ++second)
        if (*first == *second) return false;
    return true;
  }

  constexpr bool fits(const std::span<const std::string_view> names) { return names.size() <= cse::group::capacity; }

  constexpr std::size_t position(const std::span<const std::string_view> names, const std::string_view label)
  {
    std::size_t index{};
    for (const auto name : names)
    {
      if (name == label) return index;
      ++index;
    }
    throw std::out_of_range{"A tag was requested that no GROUPS declaration provides"};
  }

  constexpr bool member(const std::string_view tag, const std::string_view owner)
  { return tag.starts_with(owner) && tag.substr(owner.size()).starts_with('.'); }

  constexpr cse::group single(const std::size_t index)
  {
    if (index >= cse::group::capacity) throw std::out_of_range{"GROUPS declares more than 64 tags in total"};
    cse::group::word words{};
    words.at(index / 64) = std::uint64_t{1} << (index % 64);
    return cse::group{words};
  }

  template <const auto &names> constexpr cse::group forge(const std::string_view label)
  { return single(position(names, label)); }

  template <const auto &names> constexpr cse::group every(const std::string_view owner)
  {
    cse::group result{};
    std::size_t index{};
    for (const auto name : std::span<const std::string_view>{names})
    {
      if (member(name, owner)) result |= single(index);
      ++index;
    }
    return result;
  }

  constexpr int lowest(const cse::group &value)
  {
    int offset{};
    for (const auto word : value.words())
    {
      if (word != 0) return offset + std::countr_zero(word);
      offset += 64;
    }
    return -1;
  }

  constexpr std::size_t count(const cse::group &value)
  {
    std::size_t total{};
    for (const auto word : value.words()) total += static_cast<std::size_t>(std::popcount(word));
    return total;
  }

  template <typename action> constexpr void visit(const cse::group &value, const action &perform)
  {
    int offset{};
    for (auto word : value.words())
    {
      for (; word != 0; word &= word - 1) perform(offset + std::countr_zero(word));
      offset += 64;
    }
  }
}
