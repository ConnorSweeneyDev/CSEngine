#pragma once

#include <any>
#include <functional>
#include <typeindex>
#include <unordered_map>

#include "id.hpp"

namespace cse::helper
{
  class hooks
  {
  public:
    template <typename signature> bool has(const id name) const;
    template <typename signature> void add(const id name, const std::function<signature> &function);
    template <typename callable> void add(const id name, callable &&function);
    template <typename signature> void remove(const id name);
    template <typename signature> void clear() noexcept;
    void clear_all() noexcept;
    template <typename signature, typename... arguments> auto call(const id name, arguments &&...args) const;
    template <typename signature, typename... arguments> auto strict_call(const id name, arguments &&...args) const;

  private:
    template <typename signature> std::unordered_map<id, std::function<signature>> &get_map();
    template <typename signature> const std::unordered_map<id, std::function<signature>> &get_map() const;

  private:
    std::unordered_map<std::type_index, std::any> functions{};
  };
}

#include "hooks.inl"
