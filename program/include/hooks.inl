#pragma once

#include "hooks.hpp"

#include <any>
#include <functional>
#include <optional>
#include <type_traits>
#include <typeindex>
#include <variant>

#include "enumeration.hpp"
#include "exception.hpp"
#include "function.hpp"

namespace cse::help
{
  template <enumeration_value key> bool hooks::has(const key name) const { return entries.contains(name); }

  template <typename signature, enumeration_value key> bool hooks::has(const key name) const
  {
    auto iterator{entries.find(name)};
    if (iterator == entries.end()) return false;
    return iterator->second.type == std::type_index(typeid(signature));
  }

  template <typename signature, enumeration_value key>
  void hooks::set(const key name, const std::function<signature> &function)
  {
    entries.insert_or_assign(name, entry{function, std::type_index(typeid(signature))});
  }

  template <typename callable, enumeration_value key> void hooks::set(const key name, callable &&function)
  {
    using signature = typename trait::callable<callable>::signature;
    set<signature>(name, std::function<signature>(std::forward<callable>(function)));
  }

  template <typename signature, enumeration_value key, typename... call_arguments>
  auto hooks::call(const key name, call_arguments &&...arguments) const
  {
    using return_type = typename trait::function<signature>::return_type;
    auto iterator{entries.find(name)};
    if (iterator == entries.end())
    {
      if constexpr (std::is_void_v<return_type>) return;
      return return_type{};
    }
    const auto &function{get_function<signature>(iterator->second)};
    if constexpr (std::is_void_v<return_type>)
      function(std::forward<call_arguments>(arguments)...);
    else
      return function(std::forward<call_arguments>(arguments)...);
  }

  template <typename signature, enumeration_value key, typename... call_arguments>
  auto hooks::try_call(const key name, call_arguments &&...arguments) const
  {
    using return_type = typename trait::function<signature>::return_type;
    using optional_type = std::conditional_t<std::is_void_v<return_type>, std::monostate, return_type>;
    auto iterator{entries.find(name)};
    if (iterator == entries.end()) return std::optional<optional_type>{std::nullopt};
    const auto &function{get_function<signature>(iterator->second)};
    if constexpr (std::is_void_v<return_type>)
    {
      function(std::forward<call_arguments>(arguments)...);
      return std::optional<optional_type>{std::monostate{}};
    }
    else
      return std::optional<optional_type>{function(std::forward<call_arguments>(arguments)...)};
  }

  template <typename signature, enumeration_value key, typename... call_arguments>
  auto hooks::throw_call(const key name, call_arguments &&...arguments) const
  {
    auto iterator{entries.find(name)};
    if (iterator == entries.end()) throw exception("Attempted to call non-existent hook");
    const auto &function{get_function<signature>(iterator->second)};
    return function(std::forward<call_arguments>(arguments)...);
  }

  template <typename signature> const std::function<signature> &hooks::get_function(const entry &target) const
  {
    try
    {
      return std::any_cast<const std::function<signature> &>(target.callback);
    }
    catch (const std::bad_any_cast &)
    {
      throw exception("Attempted to call hook with incorrect function signature");
    }
  }
}
