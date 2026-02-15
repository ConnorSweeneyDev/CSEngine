#pragma once

#include <algorithm>
#include <cmath>

namespace cse
{
  enum class axis
  {
    NONE,
    X,
    Y,
    Z
  };
}

template <typename number>
  requires std::is_integral_v<number>
inline bool between(const number value, const number minimum, const number maximum)
{
  return value >= minimum && value <= maximum;
}

template <typename real>
  requires std::is_floating_point_v<real>
inline bool equal(const real left, const real right, const real epsilon = static_cast<real>(1e-5))
{
  return std::abs(left - right) <= epsilon * std::max(static_cast<real>(1), std::max(std::abs(left), std::abs(right)));
}
