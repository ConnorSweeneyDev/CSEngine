#pragma once

#include "object.hpp"

#include <memory>

template <typename object_type> std::shared_ptr<object_type> as(const std::shared_ptr<cse::object> &object)
{
  return std::static_pointer_cast<object_type>(object);
}
