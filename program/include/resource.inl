#pragma once

#include "resource.hpp"

#include <array>
#include <cstddef>

namespace cse::resource
{
  template <std::size_t count> loader<count>::loader(const std::array<binding, count> &list) : storage{list}
  { bindings = storage; }
}
