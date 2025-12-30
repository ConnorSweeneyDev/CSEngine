#pragma once

#include <any>
#include <functional>
#include <typeindex>
#include <unordered_map>

#include "id.hpp"

namespace cse::help
{
  class hook
  {
  public:
    template <typename signature> bool has(const id name) const;
    template <typename signature> void set(const id name, const std::function<signature> &function);
    template <typename callable> void set(const id name, callable &&function);
    template <typename signature> void remove(const id name);
    template <typename signature> void reset() noexcept;
    void reset() noexcept;
    template <typename signature, typename... arguments> auto call(const id name, arguments &&...args) const;
    template <typename signature, typename... arguments> auto throw_call(const id name, arguments &&...args) const;
    template <typename signature, typename... arguments> auto try_call(const id name, arguments &&...args) const;

  private:
    template <typename signature> std::unordered_map<id, std::function<signature>> &get_map();
    template <typename signature> const std::unordered_map<id, std::function<signature>> &get_map() const;

  private:
    std::unordered_map<std::type_index, std::any> functions{};
  };
}

#include "hook.inl" // IWYU pragma: keep
