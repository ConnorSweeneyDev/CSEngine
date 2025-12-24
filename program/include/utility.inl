#pragma once

#include "utility.hpp"

#include <memory>

#include "exception.hpp"

template <typename derived, typename base> std::shared_ptr<derived> as(const std::shared_ptr<base> &object)
{
  return std::static_pointer_cast<derived>(object);
}

template <typename type> std::shared_ptr<type> lock(const std::weak_ptr<type> &object)
{
  if (auto locked{object.lock()}) return locked;
  throw cse::exception("Weak pointer lock failed for nullptr object");
}
