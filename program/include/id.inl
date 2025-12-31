#pragma once

#include "id.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace cse::help
{
  constexpr id::id(const char *string_) : hash{hash_compiletime(string_)} {}

  constexpr std::uint64_t id::get_hash() const { return hash; }

  constexpr std::uint64_t id::hash_compiletime(const char *string, std::uint64_t hash)
  {
    return *string ? hash_compiletime(string + 1, (hash ^ static_cast<std::uint64_t>(*string)) * 1099511628211ULL)
                   : hash;
  }
}

inline std::size_t std::hash<cse::help::id>::operator()(const cse::help::id &id) const
{
  return static_cast<std::size_t>(id.get_hash());
}
