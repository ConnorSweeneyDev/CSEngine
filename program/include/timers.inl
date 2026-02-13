#pragma once

#include "timers.hpp"

#include <any>
#include <functional>
#include <optional>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <variant>

#include "exception.hpp"
#include "function.hpp"
#include "name.hpp"

namespace cse::help
{
  template <typename signature> bool timers::has(const name name) const
  {
    auto iterator{entries.find(name)};
    if (iterator == entries.end()) return false;
    return iterator->second.type == std::type_index(typeid(signature));
  }

  template <typename signature>
  void timers::set(const name name, const double target, const std::function<signature> &callback)
  {
    entries.insert_or_assign(name, entry{callback, std::type_index(typeid(signature)), {0.0, target}});
  }

  template <typename callable> void timers::set(const name name, const double target, callable &&callback)
  {
    using signature = typename trait::callable<callable>::signature;
    set<signature>(name, target, std::function<signature>(std::forward<callable>(callback)));
  }

  template <typename signature, typename... call_arguments>
  auto timers::call(const name name, call_arguments &&...arguments)
  {
    using return_type = typename trait::function<signature>::return_type;
    auto iterator{entries.find(name)};
    if (iterator == entries.end())
    {
      if constexpr (std::is_void_v<return_type>) return;
      return return_type{};
    }
    auto &target{iterator->second};
    if (target.time.elapsed < target.time.target)
    {
      if constexpr (std::is_void_v<return_type>) return;
      return return_type{};
    }
    if constexpr (const auto &function{get_function<signature>(target)}; std::is_void_v<return_type>)
    {
      function(std::forward<call_arguments>(arguments)...);
      entries.erase(iterator);
    }
    else
    {
      auto result{function(std::forward<call_arguments>(arguments)...)};
      entries.erase(iterator);
      return result;
    }
  }

  template <typename signature, typename... call_arguments>
  auto timers::try_call(const name name, call_arguments &&...arguments)
  {
    using return_type = typename trait::function<signature>::return_type;
    using optional_type = std::conditional_t<std::is_void_v<return_type>, std::monostate, return_type>;
    auto iterator{entries.find(name)};
    if (iterator == entries.end()) return std::optional<optional_type>{std::nullopt};
    auto &target{iterator->second};
    if (target.time.elapsed < target.time.target) return std::optional<optional_type>{std::nullopt};
    const auto &function{get_function<signature>(target)};
    if constexpr (std::is_void_v<return_type>)
    {
      function(std::forward<call_arguments>(arguments)...);
      entries.erase(iterator);
      return std::optional<optional_type>{std::monostate{}};
    }
    auto result{function(std::forward<call_arguments>(arguments)...)};
    entries.erase(iterator);
    return std::optional<optional_type>{std::move(result)};
  }

  template <typename signature, typename... call_arguments>
  auto timers::throw_call(const name name, call_arguments &&...arguments)
  {
    using return_type = typename trait::function<signature>::return_type;
    auto iterator{entries.find(name)};
    if (iterator == entries.end()) throw exception("Attempted to call non-existent timer");
    auto &target{iterator->second};
    if (target.time.elapsed < target.time.target) throw exception("Attempted to call timer before ready");
    if constexpr (const auto &function{get_function<signature>(target)}; std::is_void_v<return_type>)
    {
      function(std::forward<call_arguments>(arguments)...);
      entries.erase(iterator);
    }
    else
    {
      auto result{function(std::forward<call_arguments>(arguments)...)};
      entries.erase(iterator);
      return result;
    }
  }

  template <typename signature> const std::function<signature> &timers::get_function(const entry &target) const
  {
    try
    {
      return std::any_cast<const std::function<signature> &>(target.callback);
    }
    catch (const std::bad_any_cast &)
    {
      throw exception("Attempted to call timer with incorrect function signature");
    }
  }
}
