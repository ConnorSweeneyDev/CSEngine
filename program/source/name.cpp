#include "name.hpp"

#include <cstdint>
#include <string>
#include <string_view>

#if defined(_DEBUG)
  #include "csd/csd.hpp"
#endif

namespace cse
{
#if defined(_DEBUG)
  name::name(const char *string_) : name{std::string_view{string_}} {}

  name::name(const std::string_view string_) : hash{csd::hash_identifier(string_)}, label{string_} {}
#endif

  name::name(const std::string &string_) : name{std::string_view{string_}} {}

  name::name(std::uint64_t identifier_) : hash{identifier_} {}

  std::string name::string() const
  {
#if defined(_DEBUG)
    return label;
#else
    return std::to_string(hash);
#endif
  }

  bool name::operator==(const name &other) const { return hash == other.hash; }

  bool name::operator!=(const name &other) const { return hash != other.hash; }
}
