#pragma once

#include <memory>
#include <type_traits>
#include <typeinfo>

#include "exception.hpp"

namespace cse::trait
{
  template <typename type> struct is_shared : std::false_type
  {
  };
  template <typename type> struct is_shared<std::shared_ptr<type>> : std::true_type
  {
  };
  template <typename type> struct is_unique : std::false_type
  {
  };
  template <typename type> struct is_unique<std::unique_ptr<type>> : std::true_type
  {
  };
  template <typename type> struct is_weak : std::false_type
  {
  };
  template <typename type> struct is_weak<std::weak_ptr<type>> : std::true_type
  {
  };
  template <typename type>
  concept is_smart = is_unique<type>::value || is_shared<type>::value || is_weak<type>::value;
  template <typename type> struct smart_inner_type;
  template <typename type> struct smart_inner_type<std::shared_ptr<type>>
  {
    using extracted_type = type;
  };
  template <typename type> struct smart_inner_type<std::unique_ptr<type>>
  {
    using extracted_type = type;
  };
  template <typename type> struct smart_inner_type<std::weak_ptr<type>>
  {
    using extracted_type = type;
  };
}

template <typename... derived, typename base> bool is(const std::shared_ptr<base> &pointer) noexcept
{
  if (!pointer) return false;
  const std::type_info &typeid_base{typeid(*pointer)};
  return (... || (typeid_base == typeid(derived)));
}

template <typename... derived, typename base> bool is_a(const std::shared_ptr<base> &pointer) noexcept
{
  if (!pointer) return false;
  return (... || (dynamic_cast<derived *>(pointer.get()) != nullptr));
}

template <typename derived, typename base> std::shared_ptr<derived> as(const std::shared_ptr<base> &pointer) noexcept
{
  return std::static_pointer_cast<derived>(pointer);
}

template <typename derived, typename base>
std::shared_ptr<derived> try_as(const std::shared_ptr<base> &pointer) noexcept
{
  if (!is<derived>(pointer)) return nullptr;
  return std::static_pointer_cast<derived>(pointer);
}

template <typename derived, typename base> std::shared_ptr<derived> throw_as(const std::shared_ptr<base> &pointer)
{
  if (!is<derived>(pointer)) throw cse::exception("Invalid cast from base to derived type");
  return std::static_pointer_cast<derived>(pointer);
}

template <typename derived, typename base>
std::shared_ptr<derived> try_as_a(const std::shared_ptr<base> &pointer) noexcept
{
  return std::dynamic_pointer_cast<derived>(pointer);
}

template <typename derived, typename base> std::shared_ptr<derived> throw_as_a(const std::shared_ptr<base> &pointer)
{
  if (!is_a<derived>(pointer)) throw cse::exception("Invalid cast from base to derived type");
  return std::static_pointer_cast<derived>(pointer);
}

template <typename type> std::shared_ptr<type> try_lock(const std::weak_ptr<type> &pointer) { return pointer.lock(); }

template <typename type> std::shared_ptr<type> throw_lock(const std::weak_ptr<type> &pointer)
{
  auto locked{pointer.lock()};
  if (!locked) throw cse::exception("Weak pointer lock failed");
  return locked;
}
