#pragma once

#include "hooks.hpp"

#include <any>
#include <functional>
#include <type_traits>
#include <typeindex>
#include <unordered_map>

#include "exception.hpp"
#include "id.hpp"

namespace cse::helper
{
  template <typename type> struct function_traits;

  template <typename return_type, typename... arguments> struct function_traits<return_type(arguments...)>
  {
    using extracted_return_type = return_type;
  };

  template <typename type> struct callable_traits : callable_traits<decltype(&type::operator())>
  {
  };

  template <typename class_type, typename return_type, typename... arguments>
  struct callable_traits<return_type (class_type::*)(arguments...) const>
  {
    using signature = return_type(arguments...);
  };

  template <typename class_type, typename return_type, typename... arguments>
  struct callable_traits<return_type (class_type::*)(arguments...)>
  {
    using signature = return_type(arguments...);
  };

  template <typename return_type, typename... arguments> struct callable_traits<return_type (*)(arguments...)>
  {
    using signature = return_type(arguments...);
  };

  template <typename return_type, typename... arguments>
  struct callable_traits<std::function<return_type(arguments...)>>
  {
    using signature = return_type(arguments...);
  };

  template <typename signature> bool hooks::has(const id name) const
  {
    auto type_id{std::type_index(typeid(signature))};
    if (!functions.contains(type_id)) return false;
    const auto &map{get_map<signature>()};
    return map.contains(name);
  }

  template <typename signature> void hooks::add(const id name, const std::function<signature> &function)
  {
    auto &map{get_map<signature>()};
    if (map.contains(name)) throw utility::exception("Attempted to add duplicate hook");
    map.emplace(name, function);
  }

  template <typename callable> void hooks::add(const id name, callable &&function)
  {
    using deduced_signature = typename callable_traits<std::decay_t<callable>>::signature;
    add<deduced_signature>(name, std::function<deduced_signature>(std::forward<callable>(function)));
  }

  template <typename signature> void hooks::remove(const id name)
  {
    auto &map{get_map<signature>()};
    if (!map.contains(name)) throw utility::exception("Attempted to remove non-existent hook");
    map.erase(name);
  }

  template <typename signature> void hooks::clear() noexcept
  {
    auto type_id{std::type_index(typeid(signature))};
    if (functions.contains(type_id)) { get_map<signature>().clear(); }
  }

  inline void hooks::clear_all() noexcept { functions.clear(); }

  template <typename signature, typename... arguments> auto hooks::call(const id name, arguments &&...args) const
  {
    using extracted_return_type = typename function_traits<signature>::extracted_return_type;

    if constexpr (std::is_void_v<extracted_return_type>)
    {
      if (has<signature>(name))
      {
        const auto &map{get_map<signature>()};
        map.at(name)(std::forward<arguments>(args)...);
      }
    }
    else
    {
      if (!has<signature>(name)) return extracted_return_type{};
      const auto &map{get_map<signature>()};
      return map.at(name)(std::forward<arguments>(args)...);
    }
  }

  template <typename signature, typename... arguments> auto hooks::strict_call(const id name, arguments &&...args) const
  {
    if (!has<signature>(name)) throw utility::exception("Attempted to call non-existent hook");
    const auto &map{get_map<signature>()};
    return map.at(name)(std::forward<arguments>(args)...);
  }

  template <typename signature> std::unordered_map<id, std::function<signature>> &hooks::get_map()
  {
    auto type_id{std::type_index(typeid(signature))};
    if (!functions.contains(type_id)) { functions[type_id] = std::unordered_map<id, std::function<signature>>{}; }
    return std::any_cast<std::unordered_map<id, std::function<signature>> &>(functions[type_id]);
  }

  template <typename signature> const std::unordered_map<id, std::function<signature>> &hooks::get_map() const
  {
    auto type_id{std::type_index(typeid(signature))};
    return std::any_cast<const std::unordered_map<id, std::function<signature>> &>(functions.at(type_id));
  }
}
