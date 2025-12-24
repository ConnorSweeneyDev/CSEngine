#pragma once

#include "camera.hpp"

#include <memory>

template <typename camera_type> std::shared_ptr<camera_type> as(const std::shared_ptr<cse::camera> &camera)
{
  return std::static_pointer_cast<camera_type>(camera);
}
