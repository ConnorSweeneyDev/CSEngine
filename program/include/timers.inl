#pragma once

#include "timers.hpp"

#include <functional>
#include <utility>

#include "name.hpp"

namespace cse::help
{
  template <typename callable> void timers::set(const help::name id, const double target, callable &&callback)
  {
    entries.insert_or_assign(id, entry{std::function<void()>(std::forward<callable>(callback)), {0.0, target}});
  }
}
