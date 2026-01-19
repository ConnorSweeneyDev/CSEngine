#pragma once

#include <any>
#include <functional>
#include <typeindex>
#include <unordered_map>

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
    bool has(const int key) const;
    template <typename signature> bool has(const int key) const;
    template <typename signature> void set(const int key, const std::function<signature> &function);
    template <typename callable> void set(const int key, callable &&function);
    void remove(const int key);
    void reset() noexcept;
    template <typename signature, typename... call_arguments>
    auto call(const int key, call_arguments &&...arguments) const;
    template <typename signature, typename... call_arguments>
    auto try_call(const int key, call_arguments &&...arguments) const;
    template <typename signature, typename... call_arguments>
    auto throw_call(const int key, call_arguments &&...arguments) const;

  private:
    template <typename signature> const std::function<signature> &get_function(const entry &entry) const;

  private:
    std::unordered_map<int, entry> entries{};
  };
}

#include "hooks.inl" // IWYU pragma: keep
