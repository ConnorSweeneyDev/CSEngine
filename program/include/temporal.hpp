#pragma once

namespace cse
{
  template <typename type> class temporal
  {
  public:
    type interpolated(const temporal &previous, const double alpha) const;

  public:
    type value{};
    bool interpolate{true};

    type rate{};
    type curve{};
    bool instant{};
  };
}

#include "temporal.inl" // IWYU pragma: keep
