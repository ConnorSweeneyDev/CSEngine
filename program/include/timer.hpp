#pragma once

#include <any>
#include <functional>
#include <optional>
#include <typeindex>
#include <unordered_map>

#include "declaration.hpp"
#include "name.hpp"

namespace cse::help
{
  class timer
  {
    friend class cse::game;
    friend class cse::window;
    friend class cse::scene;
    friend class cse::camera;
    friend class cse::object;

  public:
    struct time
    {
      double elapsed{};
      double target{};
    };

  private:
    struct entry
    {
      std::any callback{};
      std::type_index type{typeid(void)};
      struct time time{};
    };

  public:
    bool has(const name name) const;
    template <typename signature> bool has(const name name) const;
    bool ready(const name name) const;
    std::optional<time> check(const name name) const;
    template <typename signature>
    void set(const name name, const double target, const std::function<signature> &callback);
    template <typename callable> void set(const name name, const double target, callable &&callback);
    void remove(const name name);
    void reset() noexcept;
    template <typename signature, typename... call_arguments> auto call(const name name, call_arguments &&...arguments);
    template <typename signature, typename... call_arguments>
    auto try_call(const name name, call_arguments &&...arguments);
    template <typename signature, typename... call_arguments>
    auto throw_call(const name name, call_arguments &&...arguments);

  private:
    template <typename signature> const std::function<signature> &get_function(const entry &entry) const;
    void update(const double tick);

  private:
    std::unordered_map<name, entry> entries{};
  };
}

#include "timer.inl" // IWYU pragma: keep
