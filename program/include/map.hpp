#pragma once

#include <memory>
#include <unordered_map>

#include "exception.hpp"
#include "name.hpp"

template <typename type> cse::help::name try_name(const std::unordered_map<cse::help::name, std::shared_ptr<type>> &map,
                                                  const std::shared_ptr<type> &pointer) noexcept
{
  if (!pointer) return cse::help::name{};
  for (const auto &[name, entry] : map)
    if (entry == pointer) return name;
  return cse::help::name{};
}

template <typename type> cse::help::name try_name(const std::unordered_map<cse::help::name, std::shared_ptr<type>> &map,
                                                  const std::weak_ptr<type> &pointer) noexcept
{
  auto locked{pointer.lock()};
  if (!locked) return cse::help::name{};
  for (const auto &[name, entry] : map)
    if (entry == locked) return name;
  return cse::help::name{};
}

template <typename type> cse::help::name
throw_name(const std::unordered_map<cse::help::name, std::shared_ptr<type>> &map, const std::shared_ptr<type> &pointer)
{
  if (!pointer) throw cse::exception("Pointer is null");
  for (const auto &[name, entry] : map)
    if (entry == pointer) return name;
  throw cse::exception("ID lookup failed");
}

template <typename type> cse::help::name
throw_name(const std::unordered_map<cse::help::name, std::shared_ptr<type>> &map, const std::weak_ptr<type> &pointer)
{
  auto locked{pointer.lock()};
  if (!locked) throw cse::exception("Weak pointer lock failed");
  for (const auto &[name, entry] : map)
    if (entry == locked) return name;
  throw cse::exception("ID lookup failed");
}

template <typename type> std::shared_ptr<type>
try_find(const std::unordered_map<cse::help::name, std::shared_ptr<type>> &map, const cse::help::name name) noexcept
{
  auto iterator{map.find(name)};
  if (iterator == map.end()) return nullptr;
  return iterator->second;
}

template <typename type> std::shared_ptr<type>
throw_find(const std::unordered_map<cse::help::name, std::shared_ptr<type>> &map, const cse::help::name name)
{
  auto iterator{map.find(name)};
  if (iterator == map.end()) throw cse::exception("Map lookup failed");
  return iterator->second;
}
