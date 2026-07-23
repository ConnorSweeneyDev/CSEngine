#pragma once

#include "name.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

#if defined(NDEBUG)
  #include <string_view>

  #include "csd/csd.hpp"
#endif

namespace cse
{
#if defined(NDEBUG)
  constexpr name::name(const char *string_) : name{std::string_view{string_}} {}

  constexpr name::name(const std::string_view string_) : hash{csd::hash_identifier(string_)} {}
#endif

  constexpr std::uint64_t name::identifier() const { return hash; }
}

inline std::size_t std::hash<cse::name>::operator()(const cse::name &name) const
{ return static_cast<std::size_t>(name.identifier()); }
