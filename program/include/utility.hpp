#pragma once

#include <memory>
#include <typeinfo>

#include "exception.hpp"

template <typename derived, typename base> bool is(const std::shared_ptr<base> &object) noexcept
{
  const std::type_info &typeid_derived{typeid(derived)};
  const std::type_info &typeid_base{typeid(*object)};
  return object && (typeid_base == typeid_derived);
}

template <typename derived, typename base> std::shared_ptr<derived> as(const std::shared_ptr<base> &object) noexcept
{
  return std::static_pointer_cast<derived>(object);
}

template <typename derived, typename base> std::shared_ptr<derived> try_as(const std::shared_ptr<base> &object) noexcept
{
  if (is<derived>(object)) return as<derived>(object);
  return nullptr;
}

template <typename type> std::shared_ptr<type> lock(const std::weak_ptr<type> &object)
{
  if (auto locked{object.lock()}) return locked;
  throw cse::exception("Weak pointer lock failed for object");
}
