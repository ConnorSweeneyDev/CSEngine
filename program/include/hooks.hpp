#pragma once

#include <any>
#include <functional>
#include <typeindex>
#include <unordered_map>

#include "wrapper.hpp"

namespace cse::help
{
  class hooks
  {
  private:
    struct entry
    {
      std::any callback{};
      std::type_index type{typeid(void)};
    };

  public:
    template <enumeration_value key> bool has(const key name) const;
    template <typename signature, enumeration_value key> bool has(const key name) const;
    template <typename signature, enumeration_value key>
    void set(const key name, const std::function<signature> &function);
    template <typename callable, enumeration_value key> void set(const key name, callable &&function);
    template <enumeration_value key> void remove(const key name);
    void reset() noexcept;
    template <typename signature, enumeration_value key, typename... call_arguments>
    auto call(const key name, call_arguments &&...arguments) const;
    template <typename signature, enumeration_value key, typename... call_arguments>
    auto try_call(const key name, call_arguments &&...arguments) const;
    template <typename signature, enumeration_value key, typename... call_arguments>
    auto throw_call(const key name, call_arguments &&...arguments) const;

  private:
    template <typename signature> const std::function<signature> &get_function(const entry &entry) const;

  private:
    std::unordered_map<int, entry> entries{};
  };
}

#include "hooks.inl" // IWYU pragma: keep
