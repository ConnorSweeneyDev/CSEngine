#pragma once

#include "hooks.hpp"

#include <any>
#include <functional>
#include <optional>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <variant>

#include "exception.hpp"
#include "traits.hpp"

namespace cse::help
{
  template <typename signature> bool hooks::has(const int key) const
  {
    auto type_id{std::type_index(typeid(signature))};
    if (!functions.contains(type_id)) return false;
    const auto &map{get_map<signature>()};
    return map.contains(key);
  }

  template <typename signature> void hooks::set(const int key, const std::function<signature> &function)
  {
    auto &map{get_map<signature>()};
    map.insert_or_assign(key, function);
  }

  template <typename callable> void hooks::set(const int key, callable &&function)
  {
    using deduced_signature = typename callable_traits<std::decay_t<callable>>::signature;
    set<deduced_signature>(key, std::function<deduced_signature>(std::forward<callable>(function)));
  }

  template <typename signature> void hooks::remove(const int key)
  {
    auto &map{get_map<signature>()};
    map.erase(key);
  }

  template <typename signature> void hooks::reset() noexcept
  {
    auto type_id{std::type_index(typeid(signature))};
    if (functions.contains(type_id)) get_map<signature>().clear();
  }

  template <typename signature, typename... arguments> auto hooks::call(const int key, arguments &&...args) const
  {
    using extracted_return_type = typename function_traits<signature>::extracted_return_type;
    auto type_id{std::type_index(typeid(signature))};
    if (!functions.contains(type_id))
    {
      if constexpr (std::is_void_v<extracted_return_type>) return;
      return extracted_return_type{};
    }
    const auto &map{get_map<signature>()};
    if (!map.contains(key))
    {
      if constexpr (std::is_void_v<extracted_return_type>) return;
      return extracted_return_type{};
    }
    if constexpr (std::is_void_v<extracted_return_type>)
      map.at(key)(std::forward<arguments>(args)...);
    else
      return map.at(key)(std::forward<arguments>(args)...);
  }

  template <typename signature, typename... arguments> auto hooks::throw_call(const int key, arguments &&...args) const
  {
    auto type_id{std::type_index(typeid(signature))};
    if (!functions.contains(type_id)) throw exception("Attempted to call non-existent hook");
    const auto &map{get_map<signature>()};
    auto it{map.find(key)};
    if (it == map.end()) throw exception("Attempted to call non-existent hook");
    return it->second(std::forward<arguments>(args)...);
  }

  template <typename signature, typename... arguments> auto hooks::try_call(const int key, arguments &&...args) const
  {
    using return_type = typename function_traits<signature>::extracted_return_type;
    using optional_type = std::conditional_t<std::is_void_v<return_type>, std::monostate, return_type>;
    auto type_id{std::type_index(typeid(signature))};
    if (!functions.contains(type_id)) return std::optional<optional_type>{std::nullopt};
    const auto &map{get_map<signature>()};
    auto it{map.find(key)};
    if (it == map.end()) return std::optional<optional_type>{std::nullopt};
    if constexpr (std::is_void_v<return_type>)
    {
      it->second(std::forward<arguments>(args)...);
      return std::optional<optional_type>{std::monostate{}};
    }
    else
      return std::optional<optional_type>{it->second(std::forward<arguments>(args)...)};
  }

  template <typename signature> std::unordered_map<int, std::function<signature>> &hooks::get_map()
  {
    auto type_id{std::type_index(typeid(signature))};
    if (!functions.contains(type_id)) functions[type_id] = std::unordered_map<int, std::function<signature>>{};
    return std::any_cast<std::unordered_map<int, std::function<signature>> &>(functions[type_id]);
  }

  template <typename signature> const std::unordered_map<int, std::function<signature>> &hooks::get_map() const
  {
    auto type_id{std::type_index(typeid(signature))};
    return std::any_cast<const std::unordered_map<int, std::function<signature>> &>(functions.at(type_id));
  }
}
