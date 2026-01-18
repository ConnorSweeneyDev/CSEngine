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
    template <typename signature, typename... arguments> auto call(const int key, arguments &&...args) const;
    template <typename signature, typename... arguments> auto throw_call(const int key, arguments &&...args) const;
    template <typename signature, typename... arguments> auto try_call(const int key, arguments &&...args) const;

  private:
    template <typename signature> const std::function<signature> &get_function(const entry &entry) const;

  private:
    std::unordered_map<int, entry> entries{};
  };
}

#include "hooks.inl" // IWYU pragma: keep
