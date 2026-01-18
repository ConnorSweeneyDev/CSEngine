#pragma once

#include <functional>
#include <optional>
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
      std::function<void()> callback{};
      struct time time{};
    };

  public:
    bool has(const help::name id) const;
    bool ready(const help::name id) const;
    std::optional<time> check(const help::name id) const;
    template <typename callable> void set(const help::name id, const double target, callable &&callback);
    void remove(const help::name id);
    void reset() noexcept;
    bool call(const help::name id);
    void throw_call(const help::name id);

  private:
    void update(const double poll_rate);

  private:
    std::unordered_map<help::name, entry> entries{};
  };
}

#include "timers.inl" // IWYU pragma: keep
