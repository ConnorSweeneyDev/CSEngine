#include "name.hpp"

#include <cstdint>
#include <string>

namespace cse
{
#if defined(_DEBUG)
  name::name(const char *string_) : hash{hash_compiletime(string_)}, label{string_} {}
#endif

  name::name(const std::string &string_)
    : hash{hash_runtime(string_)}
#if defined(_DEBUG)
      ,
      label{string_}
#endif
  {
  }

  std::string name::string() const
  {
#if defined(_DEBUG)
    return label;
#else
    return std::to_string(hash);
#endif
  }

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
