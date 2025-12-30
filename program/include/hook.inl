#pragma once

#include "hook.hpp"

#include <any>
#include <functional>
#include <optional>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <variant>

#include "exception.hpp"
#include "id.hpp"
#include "traits.hpp"

namespace cse::help
{
  template <typename signature> bool hook::has(const id name) const
  {
    auto type_id{std::type_index(typeid(signature))};
    if (!functions.contains(type_id)) return false;
    const auto &map{get_map<signature>()};
    return map.contains(name);
  }

  template <typename signature> void hook::set(const id name, const std::function<signature> &function)
  {
    auto &map{get_map<signature>()};
    map.insert_or_assign(name, function);
  }

  template <typename callable> void hook::set(const id name, callable &&function)
  {
    using deduced_signature = typename callable_traits<std::decay_t<callable>>::signature;
    set<deduced_signature>(name, std::function<deduced_signature>(std::forward<callable>(function)));
  }

  template <typename signature> void hook::remove(const id name)
  {
    auto &map{get_map<signature>()};
    map.erase(name);
  }

  template <typename signature> void hook::reset() noexcept
  {
    auto type_id{std::type_index(typeid(signature))};
    if (functions.contains(type_id)) get_map<signature>().clear();
  }

  template <typename signature, typename... arguments> auto hook::call(const id name, arguments &&...args) const
  {
    using extracted_return_type = typename function_traits<signature>::extracted_return_type;
    auto type_id{std::type_index(typeid(signature))};
    if (!functions.contains(type_id))
    {
      if constexpr (std::is_void_v<extracted_return_type>) return;
      return extracted_return_type{};
    }
    const auto &map{get_map<signature>()};
    if (!map.contains(name))
    {
      if constexpr (std::is_void_v<extracted_return_type>) return;
      return extracted_return_type{};
    }
    if constexpr (std::is_void_v<extracted_return_type>)
      map.at(name)(std::forward<arguments>(args)...);
    else
      return map.at(name)(std::forward<arguments>(args)...);
  }

  template <typename signature, typename... arguments> auto hook::throw_call(const id name, arguments &&...args) const
  {
    auto type_id{std::type_index(typeid(signature))};
    if (!functions.contains(type_id)) throw exception("Attempted to call non-existent hook");
    const auto &map{get_map<signature>()};
    auto it{map.find(name)};
    if (it == map.end()) throw exception("Attempted to call non-existent hook");
    return it->second(std::forward<arguments>(args)...);
  }

  template <typename signature, typename... arguments> auto hook::try_call(const id name, arguments &&...args) const
  {
    using return_type = typename function_traits<signature>::extracted_return_type;
    using optional_type = std::conditional_t<std::is_void_v<return_type>, std::monostate, return_type>;
    auto type_id{std::type_index(typeid(signature))};
    if (!functions.contains(type_id)) return std::optional<optional_type>{std::nullopt};
    const auto &map{get_map<signature>()};
    auto it{map.find(name)};
    if (it == map.end()) return std::optional<optional_type>{std::nullopt};
    if constexpr (std::is_void_v<return_type>)
    {
      it->second(std::forward<arguments>(args)...);
      return std::optional<optional_type>{std::monostate{}};
    }
    else
      return std::optional<optional_type>{it->second(std::forward<arguments>(args)...)};
  }

  template <typename signature> std::unordered_map<id, std::function<signature>> &hook::get_map()
  {
    auto type_id{std::type_index(typeid(signature))};
    if (!functions.contains(type_id)) functions[type_id] = std::unordered_map<id, std::function<signature>>{};
    return std::any_cast<std::unordered_map<id, std::function<signature>> &>(functions[type_id]);
  }

  template <typename signature> const std::unordered_map<id, std::function<signature>> &hook::get_map() const
  {
    auto type_id{std::type_index(typeid(signature))};
    return std::any_cast<const std::unordered_map<id, std::function<signature>> &>(functions.at(type_id));
  }
}
