#pragma once

#include "enumeration.hpp"

#include <optional>

namespace cse
{
  template <typename derived> enumeration<derived>::value::value() : count{next()} {}

  template <typename derived> enumeration<derived>::value::value(int count_) : count{next(count_)} {}

  template <typename derived> enumeration<derived>::value::operator int() const noexcept { return count; }

  template <typename derived> bool enumeration<derived>::value::operator==(const value &other_) const noexcept
  {
    return count == other_.count;
  }

  template <typename derived> auto enumeration<derived>::value::operator<=>(const value &other_) const noexcept
  {
    return count <=> other_.count;
  }

  template <typename derived> int enumeration<derived>::next(std::optional<int> count)
  {
    static int counter{};
    if (count.has_value())
    {
      counter = count.value() + 1;
      return count.value();
    }
    return counter++;
  }
}
