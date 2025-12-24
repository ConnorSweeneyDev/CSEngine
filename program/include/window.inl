#pragma once

#include "window.hpp"

#include <memory>

template <typename window_type> std::shared_ptr<window_type> as(const std::shared_ptr<cse::window> &window)
{
  return std::static_pointer_cast<window_type>(window);
}
