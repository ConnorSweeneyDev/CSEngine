#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <typeinfo>
#include <unordered_map>

#include "exception.hpp"
#include "id.hpp"

template <typename derived, typename base> std::shared_ptr<derived> as(const std::shared_ptr<base> &pointer) noexcept
{
  return std::static_pointer_cast<derived>(pointer);
}

template <typename... derived, typename base> bool is(const std::shared_ptr<base> &pointer) noexcept
{
  if (!pointer) return false;
  const std::type_info &typeid_base{typeid(*pointer)};
  return (... || (typeid_base == typeid(derived)));
}

template <typename derived, typename base> std::shared_ptr<derived> throw_as(const std::shared_ptr<base> &pointer)
{
  if (is<derived>(pointer)) return std::static_pointer_cast<derived>(pointer);
  throw cse::exception("Invalid cast from base to derived type");
}

template <typename derived, typename base>
std::shared_ptr<derived> try_as(const std::shared_ptr<base> &pointer) noexcept
{
  if (is<derived>(pointer)) return std::static_pointer_cast<derived>(pointer);
  return nullptr;
}

template <typename... derived, typename base> bool is_a(const std::shared_ptr<base> &pointer) noexcept
{
  if (!pointer) return false;
  return (... || (dynamic_cast<derived *>(pointer.get()) != nullptr));
}

template <typename derived, typename base> std::shared_ptr<derived> throw_as_a(const std::shared_ptr<base> &pointer)
{
  if (is_a<derived>(pointer)) return std::static_pointer_cast<derived>(pointer);
  throw cse::exception("Invalid cast from base to derived type");
}

template <typename derived, typename base>
std::shared_ptr<derived> try_as_a(const std::shared_ptr<base> &pointer) noexcept
{
  if (is_a<derived>(pointer)) return std::static_pointer_cast<derived>(pointer);
  return nullptr;
}

template <typename type>
cse::help::id try_id(const std::shared_ptr<type> &pointer,
                     const std::unordered_map<cse::help::id, std::shared_ptr<type>> &map) noexcept
{
  for (const auto &[name, entry] : map)
    if (entry == pointer) return name;
  return cse::help::id{};
}

template <typename type> cse::help::id
try_id(const std::weak_ptr<type> &pointer, const std::unordered_map<cse::help::id, std::shared_ptr<type>> &map) noexcept
{
  auto locked{pointer.lock()};
  if (!locked) return cse::help::id{};
  for (const auto &[name, entry] : map)
    if (entry == locked) return name;
  return cse::help::id{};
}

template <typename type> cse::help::id throw_id(const std::shared_ptr<type> &pointer,
                                                const std::unordered_map<cse::help::id, std::shared_ptr<type>> &map)
{
  for (const auto &[name, entry] : map)
    if (entry == pointer) return name;
  throw cse::exception("ID lookup failed");
}

template <typename type> cse::help::id throw_id(const std::weak_ptr<type> &pointer,
                                                const std::unordered_map<cse::help::id, std::shared_ptr<type>> &map)
{
  auto locked{pointer.lock()};
  if (!locked) throw cse::exception("Weak pointer lock failed");
  for (const auto &[name, entry] : map)
    if (entry == locked) return name;
  throw cse::exception("ID lookup failed");
}

template <typename type> std::shared_ptr<type>
try_at(const std::unordered_map<cse::help::id, std::shared_ptr<type>> &map, const cse::help::id name) noexcept
{
  auto iterator{map.find(name)};
  if (iterator != map.end()) return iterator->second;
  return nullptr;
}

template <typename type> std::shared_ptr<type>
throw_at(const std::unordered_map<cse::help::id, std::shared_ptr<type>> &map, const cse::help::id name)
{
  auto iterator{map.find(name)};
  if (iterator != map.end()) return iterator->second;
  throw cse::exception("Map lookup failed");
}

template <typename type> std::shared_ptr<type> throw_lock(const std::weak_ptr<type> &pointer)
{
  if (auto locked{pointer.lock()}) return locked;
  throw cse::exception("Weak pointer lock failed");
}

inline bool equal(const double first, const double second, const double epsilon = 1e-5)
{
  return std::fabs(first - second) <= epsilon * std::max(1.0, std::max(std::fabs(first), std::fabs(second)));
}
