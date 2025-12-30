#pragma once

#include <memory>
#include <typeinfo>

#include "exception.hpp"

template <typename derived, typename base> std::shared_ptr<derived> as(const std::shared_ptr<base> &object) noexcept
{
  return std::static_pointer_cast<derived>(object);
}

template <typename... derived, typename base> bool is(const std::shared_ptr<base> &object) noexcept
{
  if (!object) return false;
  const std::type_info &typeid_base{typeid(*object)};
  return (... || (typeid_base == typeid(derived)));
}

template <typename derived, typename base> std::shared_ptr<derived> throw_as(const std::shared_ptr<base> &object)
{
  if (is<derived>(object)) return std::static_pointer_cast<derived>(object);
  throw cse::exception("Invalid cast from base to derived type");
}

template <typename derived, typename base> std::shared_ptr<derived> try_as(const std::shared_ptr<base> &object) noexcept
{
  if (is<derived>(object)) return std::static_pointer_cast<derived>(object);
  return nullptr;
}

template <typename... derived, typename base> bool is_a(const std::shared_ptr<base> &object) noexcept
{
  if (!object) return false;
  return (... || (dynamic_cast<derived *>(object.get()) != nullptr));
}

template <typename derived, typename base> std::shared_ptr<derived> throw_as_a(const std::shared_ptr<base> &object)
{
  if (is_a<derived>(object)) return std::static_pointer_cast<derived>(object);
  throw cse::exception("Invalid cast from base to derived type");
}

template <typename derived, typename base>
std::shared_ptr<derived> try_as_a(const std::shared_ptr<base> &object) noexcept
{
  if (is_a<derived>(object)) return std::static_pointer_cast<derived>(object);
  return nullptr;
}

template <typename type> std::shared_ptr<type> throw_lock(const std::weak_ptr<type> &object)
{
  if (auto locked{object.lock()}) return locked;
  throw cse::exception("Weak pointer lock failed for object");
}
