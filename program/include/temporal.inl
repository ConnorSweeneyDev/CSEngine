#pragma once

#include "temporal.hpp"

#include <type_traits>

namespace cse
{
  template <typename type> temporal<type>::temporal(const type &value_) : value{value_} {}

  template <typename type> template <typename... arguments>
    requires(sizeof...(arguments) != 1 || (!std::is_same_v<std::decay_t<arguments>, type> && ...))
  temporal<type>::temporal(arguments &&...arguments_) : value{std::forward<arguments>(arguments_)...}
  {
  }
}
