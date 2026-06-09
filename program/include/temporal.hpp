#pragma once

#include <type_traits>

namespace cse
{
  template <typename type> class temporal
  {
  public:
    temporal() = default;
    temporal(const type &value_);
    template <typename... arguments>
      requires(sizeof...(arguments) != 1 || (!std::is_same_v<std::decay_t<arguments>, type> && ...))
    temporal(arguments &&...arguments_);

  public:
    type value{};
    type rate{};
    type curve{};
  };
}

#include "temporal.inl" // IWYU pragma: keep
