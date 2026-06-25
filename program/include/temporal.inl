#pragma once

#include "temporal.hpp"

namespace cse
{
  template <typename type> type temporal<type>::interpolated(const temporal &previous, const double alpha) const
  {
    if (!interpolate || instant) return value;
    return previous.value + (value - previous.value) * alpha;
  }
}
