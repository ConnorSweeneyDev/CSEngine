#pragma once

#include <any>
#include <cstddef>
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
      bool repeat{};
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
    std::size_t count() const noexcept;
    bool has(const name name) const;
    template <typename signature> bool has(const name name) const;
    state &get(const name name);
    const state &get(const name name) const;
    template <typename signature> state &set(const name name, const std::function<signature> &callback);
    template <typename callable> state &set(const name name, callable &&callback);
    template <typename callable> void iterate(callable &&function);
    template <typename callable> void iterate(callable &&function) const;
    void remove(const name name);
    void remove(std::initializer_list<name> names);
    void clear() noexcept;

    bool ready(const name name) const;
    template <typename signature, typename... call_arguments> bool call(const name name, call_arguments &&...arguments);
    template <typename signature, typename... call_arguments>
    auto capture(const name name, call_arguments &&...arguments);

  private:
    template <typename signature> const std::function<signature> &deduce(const name name, const entry &target) const;
    void finish(std::unordered_map<name, entry>::iterator iterator);
    void update(const double tick);

  private:
    std::unordered_map<name, entry> entries{};
  };
}

#include "timer.inl" // IWYU pragma: keep
