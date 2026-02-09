#pragma once

#include <concepts>
#include <iterator>
#include <memory>

#include "exception.hpp"
#include "name.hpp"

template <typename map, typename value>
concept pointer_map = requires(const map &container, cse::help::name identifier) {
  { container.find(identifier) } -> std::input_iterator;
  { container.end() } -> std::sentinel_for<decltype(container.find(identifier))>;
  { container.find(identifier)->second } -> std::convertible_to<std::shared_ptr<value>>;
  { container.begin() } -> std::input_iterator;
};

template <typename value, pointer_map<value> map>
cse::help::name try_name(const map &container, const std::shared_ptr<value> &pointer) noexcept
{
  if (!pointer) return cse::help::name{};
  for (const auto &[name, entry] : container)
    if (entry == pointer) return name;
  return cse::help::name{};
}

template <typename value, pointer_map<value> map>
cse::help::name try_name(const map &container, const std::weak_ptr<value> &pointer) noexcept
{
  auto locked{pointer.lock()};
  if (!locked) return cse::help::name{};
  for (const auto &[name, entry] : container)
    if (entry == locked) return name;
  return cse::help::name{};
}

template <typename value, pointer_map<value> map>
cse::help::name throw_name(const map &container, const std::shared_ptr<value> &pointer)
{
  if (!pointer) throw cse::exception("Pointer is null");
  for (const auto &[name, entry] : container)
    if (entry == pointer) return name;
  throw cse::exception("Key lookup failed");
}

template <typename value, pointer_map<value> map>
cse::help::name throw_name(const map &container, const std::weak_ptr<value> &pointer)
{
  auto locked{pointer.lock()};
  if (!locked) throw cse::exception("Weak pointer lock failed");
  for (const auto &[name, entry] : container)
    if (entry == locked) return name;
  throw cse::exception("Key lookup failed");
}

template <typename value, pointer_map<value> map>
std::shared_ptr<value> try_find(const map &container, const cse::help::name identifier) noexcept
{
  auto iterator{container.find(identifier)};
  if (iterator == container.end()) return nullptr;
  return iterator->second;
}

template <typename value, pointer_map<value> map>
std::shared_ptr<value> throw_find(const map &container, const cse::help::name identifier)
{
  auto iterator{container.find(identifier)};
  if (iterator == container.end()) throw cse::exception("Map lookup failed");
  return iterator->second;
}
