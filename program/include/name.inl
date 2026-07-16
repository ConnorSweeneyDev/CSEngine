#pragma once

#include "name.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>

namespace cse
{
#if defined(NDEBUG)
  constexpr name::name(const char *string_) : name{std::string_view{string_}} {}

  constexpr name::name(const std::string_view string_) : hash{hash_string(string_)} {}
#endif

  constexpr std::uint64_t name::identifier() const { return hash; }

  constexpr std::uint64_t name::hash_string(const std::string_view string)
  {
    std::uint64_t hash{14695981039346656037ULL};
    for (const char character : string)
    {
      hash ^= static_cast<std::uint64_t>(character);
      hash *= 1099511628211ULL;
    }
    return hash;
  }
}

inline std::size_t std::hash<cse::name>::operator()(const cse::name &name) const
{ return static_cast<std::size_t>(name.identifier()); }
