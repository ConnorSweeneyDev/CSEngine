#pragma once

#include <algorithm>
#include <concepts>
#include <iterator>
#include <memory>
#include <utility>

#include "exception.hpp"
#include "name.hpp"
#include "pointer.hpp"

namespace cse::trait
{
  template <typename vector>
  concept pointer_vector = requires(const vector &container) {
    { container.begin() } -> std::input_iterator;
    { container.end() } -> std::sentinel_for<decltype(container.begin())>;
    typename vector::value_type;
    requires is_shared<typename vector::value_type>::value;
    requires requires(const typename vector::value_type &element) {
      { element->state.name } -> std::convertible_to<cse::name>;
    };
  };

  template <typename set>
  concept name_set = requires(const set &container) {
    { container.find(std::declval<cse::name>()) } -> std::input_iterator;
    { container.end() } -> std::sentinel_for<decltype(container.find(std::declval<cse::name>()))>;
    typename set::value_type;
    requires std::same_as<typename set::value_type, cse::name>;
  };
}

template <cse::trait::pointer_vector vector> auto try_iterate(const vector &container, const cse::name name) noexcept
{
  return std::ranges::find_if(container, [&](const auto &element) { return element->state.name == name; });
}

template <cse::trait::name_set set> auto try_iterate(const set &container, const cse::name name) noexcept
{
  return container.find(name);
}

template <cse::trait::pointer_vector vector> auto throw_iterate(const vector &container, const cse::name name)
{
  auto iterator{std::ranges::find_if(container, [&](const auto &element) { return element->state.name == name; })};
  if (iterator == container.end()) throw cse::exception("Vector lookup for name '{}' failed", name.string());
  return iterator;
}

template <cse::trait::name_set set> auto throw_iterate(const set &container, const cse::name name)
{
  auto iterator{container.find(name)};
  if (iterator == container.end()) throw cse::exception("Set lookup for name '{}' failed", name.string());
  return iterator;
}

template <cse::trait::pointer_vector vector>
typename vector::value_type try_find(const vector &container, const cse::name name) noexcept
{
  auto iterator{std::ranges::find_if(container, [&](const auto &element) { return element->state.name == name; })};
  if (iterator == container.end()) return nullptr;
  return *iterator;
}

template <cse::trait::name_set set> cse::name try_find(const set &container, const cse::name name) noexcept
{
  auto iterator{container.find(name)};
  if (iterator == container.end()) return {};
  return *iterator;
}

template <cse::trait::pointer_vector vector>
typename vector::value_type throw_find(const vector &container, const cse::name name)
{
  auto iterator{std::ranges::find_if(container, [&](const auto &element) { return element->state.name == name; })};
  if (iterator == container.end()) throw cse::exception("Vector lookup for name '{}' failed", name.string());
  return *iterator;
}

template <cse::trait::name_set set> cse::name throw_find(const set &container, const cse::name name)
{
  auto iterator{container.find(name)};
  if (iterator == container.end()) throw cse::exception("Set lookup for name '{}' failed", name.string());
  return *iterator;
}

template <cse::trait::pointer_vector vector> bool try_contains(const vector &container, const cse::name name) noexcept
{
  return std::ranges::any_of(container, [&](const auto &element) { return element->state.name == name; });
}

template <cse::trait::name_set set> bool try_contains(const set &container, const cse::name name) noexcept
{
  return container.find(name) != container.end();
}

template <cse::trait::pointer_vector vector> bool throw_contains(const vector &container, const cse::name name)
{
  if (!std::ranges::any_of(container, [&](const auto &element) { return element->state.name == name; }))
    throw cse::exception("Vector lookup for name '{}' failed", name.string());
  return true;
}

template <cse::trait::name_set set> bool throw_contains(const set &container, const cse::name name)
{
  if (container.find(name) == container.end()) throw cse::exception("Set lookup for name '{}' failed", name.string());
  return true;
}

template <cse::trait::pointer_vector vector>
void set_or_add(vector &container, const typename vector::value_type &element)
{
  auto iterator{
    std::ranges::find_if(container, [&](const auto &existing) { return existing->state.name == element->state.name; })};
  if (iterator != container.end())
    *iterator = element;
  else
    container.push_back(element);
}

template <cse::trait::name_set set> void set_or_add(set &container, const cse::name &element)
{
  container.insert(element);
}

template <cse::trait::pointer_vector vector>
cse::name try_name(const vector &container, const typename vector::value_type &pointer) noexcept
{
  if (!pointer) return cse::name{};
  for (const auto &element : container)
    if (element == pointer) return element->state.name;
  return cse::name{};
}

template <cse::trait::pointer_vector vector> cse::name
try_name(const vector &container, const std::weak_ptr<typename vector::value_type::element_type> &pointer) noexcept
{
  auto locked{pointer.lock()};
  if (!locked) return cse::name{};
  for (const auto &element : container)
    if (element == locked) return element->state.name;
  return cse::name{};
}

template <cse::trait::pointer_vector vector>
cse::name throw_name(const vector &container, const typename vector::value_type &pointer)
{
  if (!pointer) throw cse::exception("Pointer is null");
  for (const auto &element : container)
    if (element == pointer) return element->state.name;
  throw cse::exception("Name lookup failed");
}

template <cse::trait::pointer_vector vector>
cse::name throw_name(const vector &container, const std::weak_ptr<typename vector::value_type::element_type> &pointer)
{
  auto locked{pointer.lock()};
  if (!locked) throw cse::exception("Weak pointer lock failed");
  for (const auto &element : container)
    if (element == locked) return element->state.name;
  throw cse::exception("Name lookup failed");
}
