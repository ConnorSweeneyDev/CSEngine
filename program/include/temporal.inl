#pragma once

#include "temporal.hpp"

namespace cse
{
  template <typename type> temporal<type>::temporal(const type &value_) : value{value_} {}
}
