#pragma once

#include "hooks.hpp"

#include <any>
#include <functional>
#include <optional>
#include <type_traits>
#include <typeindex>
#include <variant>

#include "exception.hpp"
#include "traits.hpp"

namespace cse::help
{
  template <typename signature> bool hooks::has(const int key) const
  {
    auto iterator{entries.find(key)};
    if (iterator == entries.end()) return false;
    return iterator->second.type == std::type_index(typeid(signature));
  }

  template <typename signature> void hooks::set(const int key, const std::function<signature> &function)
  {
    entries.insert_or_assign(key, entry{function, std::type_index(typeid(signature))});
  }

  template <typename callable> void hooks::set(const int key, callable &&function)
  {
    using deduced_signature = typename callable_traits<std::decay_t<callable>>::signature;
    set<deduced_signature>(key, std::function<deduced_signature>(std::forward<callable>(function)));
  }

  template <typename signature, typename... call_arguments>
  auto hooks::call(const int key, call_arguments &&...arguments) const
  {
    using extracted_return_type = typename function_traits<signature>::extracted_return_type;
    auto iterator{entries.find(key)};
    if (iterator == entries.end())
    {
      if constexpr (std::is_void_v<extracted_return_type>) return;
      return extracted_return_type{};
    }
    const auto &function{get_function<signature>(iterator->second)};
    if constexpr (std::is_void_v<extracted_return_type>)
      function(std::forward<call_arguments>(arguments)...);
    else
      return function(std::forward<call_arguments>(arguments)...);
  }

  template <typename signature, typename... call_arguments>
  auto hooks::try_call(const int key, call_arguments &&...arguments) const
  {
    using return_type = typename function_traits<signature>::extracted_return_type;
    using optional_type = std::conditional_t<std::is_void_v<return_type>, std::monostate, return_type>;
    auto iterator{entries.find(key)};
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

  template <typename signature, typename... call_arguments>
  auto hooks::throw_call(const int key, call_arguments &&...arguments) const
  {
    auto iterator{entries.find(key)};
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
