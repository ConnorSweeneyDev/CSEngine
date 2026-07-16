#pragma once

#include "timer.hpp"

#include <any>
#include <functional>
#include <optional>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "function.hpp"
#include "name.hpp"

namespace cse::help
{
  template <typename signature> bool timer::has(const name name) const
  {
    auto iterator{entries.find(name)};
    if (iterator == entries.end()) return false;
    return iterator->second.type == std::type_index(typeid(signature));
  }

  template <typename signature> timer::state &timer::set(const name name, const std::function<signature> &callback)
  {
    return entries.insert_or_assign(name, entry{callback, std::type_index(typeid(signature)), {}}).first->second.state;
  }

  template <typename callable> timer::state &timer::set(const name name, callable &&callback)
  {
    using signature = trait::callable<callable>::signature;
    return set<signature>(name, std::function<signature>(std::forward<callable>(callback)));
  }

  template <typename callable> void timer::iterate(callable &&function)
  {
    std::vector<name> names{};
    names.reserve(entries.size());
    for (const auto &[name, target] : entries) names.push_back(name);
    for (const auto name : names)
      if (auto iterator{entries.find(name)}; iterator != entries.end()) function(name, iterator->second.state);
  }

  template <typename callable> void timer::iterate(callable &&function) const
  {
    std::vector<name> names{};
    names.reserve(entries.size());
    for (const auto &[name, target] : entries) names.push_back(name);
    for (const auto name : names)
      if (auto iterator{entries.find(name)}; iterator != entries.end()) function(name, iterator->second.state);
  }

  template <typename signature, typename... call_arguments>
  bool timer::call(const name name, call_arguments &&...arguments)
  {
    auto iterator{entries.find(name)};
    if (iterator == entries.end()) return false;
    auto &target{iterator->second};
    if (target.state.elapsed < target.state.target) return false;
    const auto &function{deduce<signature>(name, target)};
    static_cast<void>(function(std::forward<call_arguments>(arguments)...));
    finish(iterator);
    return true;
  }

  template <typename signature, typename... call_arguments>
  auto timer::capture(const name name, call_arguments &&...arguments)
  {
    using return_type = trait::function<signature>::return_type;
    static_assert(!std::is_void_v<return_type>, "timer::capture requires a non-void callback signature");
    auto iterator{entries.find(name)};
    if (iterator == entries.end()) return std::optional<return_type>{std::nullopt};
    auto &target{iterator->second};
    if (target.state.elapsed < target.state.target) return std::optional<return_type>{std::nullopt};
    auto result{deduce<signature>(name, target)(std::forward<call_arguments>(arguments)...)};
    finish(iterator);
    return std::optional<return_type>{std::move(result)};
  }

  template <typename signature>
  const std::function<signature> &timer::deduce(const name name, const entry &target) const
  {
    try
    {
      return std::any_cast<const std::function<signature> &>(target.callback);
    }
    catch (const std::bad_any_cast &)
    {
      throw exception("Attempted to call timer '{}' with incorrect function signature", name.string());
    }
  }
}
