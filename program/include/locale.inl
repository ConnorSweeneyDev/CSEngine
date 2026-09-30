#pragma once

#include "locale.hpp"

#include <iterator>
#include <string_view>

namespace cse::help::locale
{
  template <const auto &values, const auto &languages> key forge(const std::string_view label)
  {
    static_assert(std::size(values) == std::size(languages),
                  "A translation key must give exactly one value per declared language, in LANGUAGES order");
    return key{label, values};
  }
}
