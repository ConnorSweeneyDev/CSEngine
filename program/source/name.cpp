#include "name.hpp"

#include <cstdint>
#include <string>

namespace cse
{
  name::name(const std::string &string_) : hash{hash_runtime(string_)} {}

  bool name::operator==(const name &other) const { return hash == other.hash; }

  std::uint64_t name::hash_runtime(const std::string &string)
  {
    std::uint64_t hash{14695981039346656037ULL};
    for (char character : string)
    {
      hash ^= static_cast<std::uint64_t>(character);
      hash *= 1099511628211ULL;
    }
    return hash;
  }
}
