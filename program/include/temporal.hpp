#pragma once

namespace cse
{
  template <typename type> class temporal
  {
  public:
    temporal() = default;
    temporal(const type &value_);

  public:
    type value{};
    type rate{};
    type curve{};
  };
}

#include "temporal.inl" // IWYU pragma: keep
