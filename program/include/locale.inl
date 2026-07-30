#pragma once

#include "locale.hpp"

#include <iterator>
#include <span>
#include <string_view>

namespace cse::help::locale
{
  constexpr bool distinct(const std::span<const key::entry> entries)
  {
    for (auto first{entries.begin()}; first != entries.end(); ++first)
      for (auto second{std::next(first)}; second != entries.end(); ++second)
        if (first->language == second->language) return false;
    return true;
  }

  constexpr bool complete(const std::span<const key::entry> entries, const std::span<const std::string_view> languages)
  {
    for (const auto language : languages)
    {
      bool found{false};
      for (const auto &entry : entries)
        if (entry.language == language) found = true;
      if (!found) return false;
    }
    return true;
  }

  template <const auto &entries, const auto &languages> key forge(const std::string_view label)
  {
    static_assert(distinct(entries), "A translation key declares more than one value for the same language");
    static_assert(complete(entries, languages), "A translation key is missing a value for a declared language");
    return key{label, entries};
  }
}
