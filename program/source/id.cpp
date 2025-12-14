#include "id.hpp"

#include <cstdint>
#include <string>

namespace cse::helper
{
  id::id(const std::string &string_) : hash(hash_runtime(string_)) {}

  bool id::operator==(const id &other) const { return hash == other.hash; }

  std::uint64_t id::hash_runtime(const std::string &string)
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
