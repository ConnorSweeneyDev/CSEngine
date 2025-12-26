#pragma once

#include "object.hpp"

#include <cstddef>

#include "utility.hpp"

namespace cse
{
  template <typename object_type> std::size_t object_as<object_type>::get_type_id() const noexcept
  {
    return type_id_generator::get<object_type>();
  }
}
