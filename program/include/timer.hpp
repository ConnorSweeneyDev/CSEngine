#pragma once

#include <any>
#include <functional>
#include <initializer_list>
#include <typeindex>
#include <unordered_map>

#include "core.hpp"
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
    friend class cse::light;
    friend class cse::interface;

  public:
    struct state
    {
      double elapsed{};
      double target{};
      bool running{true};
    };

  private:
    struct entry
    {
      std::any callback{};
      std::type_index type{typeid(void)};
      timer::state state{};
    };

  public:
    bool has(const name name) const;
    template <typename signature> bool has(const name name) const;
    state &get(const name name);
    const state &get(const name name) const;
    template <typename signature> void set(const name name, const double elapsed, const double target,
                                           const bool running, const std::function<signature> &callback);
    template <typename callable>
    void set(const name name, const double elapsed, const double target, const bool running, callable &&callback);
    template <typename callable> void iterate(callable &&function);
    template <typename callable> void iterate(callable &&function) const;
    void remove(const name name);
    void remove(std::initializer_list<name> names);
    void clear() noexcept;

    bool ready(const name name) const;
    template <typename signature, typename... call_arguments> auto poll(const name name, call_arguments &&...arguments);
    template <typename signature, typename... call_arguments> auto call(const name name, call_arguments &&...arguments);
    template <typename signature, typename... call_arguments>
    auto try_call(const name name, call_arguments &&...arguments);

  private:
    template <typename signature> const std::function<signature> &deduce(const name name, const entry &target) const;
    void update(const double tick);

  private:
    std::unordered_map<name, entry> entries{};
  };
}

#include "timer.inl" // IWYU pragma: keep
