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
  class timers
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
    bool has(const help::name name) const;
    template <typename signature> bool has(const help::name name) const;
    bool ready(const help::name name) const;
    std::optional<time> check(const help::name name) const;
    template <typename signature>
    void set(const help::name name, const double target, const std::function<signature> &callback);
    template <typename callable> void set(const help::name name, const double target, callable &&callback);
    void remove(const help::name name);
    void reset() noexcept;
    template <typename signature, typename... arguments> auto call(const help::name name, arguments &&...args);
    template <typename signature, typename... arguments> auto try_call(const help::name name, arguments &&...args);
    template <typename signature, typename... arguments> auto throw_call(const help::name name, arguments &&...args);

  private:
    template <typename signature> const std::function<signature> &get_function(const entry &entry) const;
    void update(const double poll_rate);

  private:
    std::unordered_map<help::name, entry> entries{};
  };
}

#include "timers.inl" // IWYU pragma: keep
