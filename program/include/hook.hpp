#pragma once

#include <any>
#include <functional>
#include <typeindex>
#include <unordered_map>

#include "name.hpp"

namespace cse::help
{
  class hook
  {
  public:
    template <typename signature> bool has(const name name) const;
    template <typename signature> void set(const name name, const std::function<signature> &function);
    template <typename callable> void set(const name name, callable &&function);
    template <typename signature> void remove(const name name);
    template <typename signature> void reset() noexcept;
    void reset() noexcept;
    template <typename signature, typename... arguments> auto call(const name name, arguments &&...args) const;
    template <typename signature, typename... arguments> auto throw_call(const name name, arguments &&...args) const;
    template <typename signature, typename... arguments> auto try_call(const name name, arguments &&...args) const;

  private:
    template <typename signature> std::unordered_map<name, std::function<signature>> &get_map();
    template <typename signature> const std::unordered_map<name, std::function<signature>> &get_map() const;

  private:
    std::unordered_map<std::type_index, std::any> functions{};
  };
}

#include "hook.inl" // IWYU pragma: keep
