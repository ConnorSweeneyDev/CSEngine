#pragma once

#include <iterator>
#include <memory>

#include "exception.hpp"
#include "name.hpp"
#include "pointer.hpp"

namespace cse::trait
{
  template <typename map>
  concept pointer_map = requires(const map &container, cse::name identifier) {
    { container.find(identifier) } -> std::input_iterator;
    { container.end() } -> std::sentinel_for<decltype(container.find(identifier))>;
    { container.begin() } -> std::input_iterator;
    typename map::mapped_type;
    requires is_smart<typename map::mapped_type>;
  };
}

template <cse::trait::pointer_map map>
cse::name try_name(const map &container, const typename map::mapped_type &pointer) noexcept
{
  if (!pointer) return cse::name{};
  for (const auto &[name, entry] : container)
    if (entry == pointer) return name;
  return cse::name{};
}

template <cse::trait::pointer_map map>
cse::name try_name(const map &container, const std::weak_ptr<typename map::mapped_type::element_type> &pointer) noexcept
{
  auto locked{pointer.lock()};
  if (!locked) return cse::name{};
  for (const auto &[name, entry] : container)
    if (entry == locked) return name;
  return cse::name{};
}

template <cse::trait::pointer_map map>
cse::name throw_name(const map &container, const typename map::mapped_type &pointer)
{
  if (!pointer) throw cse::exception("Pointer is null");
  for (const auto &[name, entry] : container)
    if (entry == pointer) return name;
  throw cse::exception("Key lookup failed");
}

template <cse::trait::pointer_map map>
cse::name throw_name(const map &container, const std::weak_ptr<typename map::mapped_type::element_type> &pointer)
{
  auto locked{pointer.lock()};
  if (!locked) throw cse::exception("Weak pointer lock failed");
  for (const auto &[name, entry] : container)
    if (entry == locked) return name;
  throw cse::exception("Key lookup failed");
}

template <cse::trait::pointer_map map>
typename map::mapped_type try_find(const map &container, const cse::name identifier) noexcept
{
  auto iterator{container.find(identifier)};
  if (iterator == container.end()) return nullptr;
  return iterator->second;
}

template <cse::trait::pointer_map map>
typename map::mapped_type throw_find(const map &container, const cse::name identifier)
{
  auto iterator{container.find(identifier)};
  if (iterator == container.end()) throw cse::exception("Map lookup failed");
  return iterator->second;
}
