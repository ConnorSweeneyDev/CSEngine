#pragma once

#include "camera.hpp"

#include <cstddef>

#include "utility.hpp"

namespace cse
{
  template <typename camera_type> std::size_t camera_as<camera_type>::get_type_id() const noexcept
  {
    return type_id_generator::get<camera_type>();
  }
}
