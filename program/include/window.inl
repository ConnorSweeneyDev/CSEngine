#pragma once

#include "window.hpp"

#include <cstddef>

#include "utility.hpp"

namespace cse
{
  template <typename window_type> std::size_t window_as<window_type>::get_type_id() const noexcept
  {
    return type_id_generator::get<window_type>();
  }
}
