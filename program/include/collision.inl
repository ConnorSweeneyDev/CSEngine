#pragma once

#include "collision.hpp"

#include "name.hpp"

namespace cse::help
{
  template <typename callable> void collisions::check(const name target, callable &&config) const
  {
    if (auto iterator{entries.find(target)}; iterator != entries.end())
      for (const auto &[mine, theirs] : iterator->second) config(mine, theirs);
  }
}
