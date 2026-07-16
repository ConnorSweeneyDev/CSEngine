#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace cse
{
  class name
  {
  public:
    name() = default;
#if defined(NDEBUG)
    constexpr
#endif
      name(const char *string_);
    name(const std::string &string_);
#if defined(NDEBUG)
    constexpr
#endif
      name(std::string_view string_);
    name(std::uint64_t identifier_);

    bool operator==(const name &other) const;
    bool operator!=(const name &other) const;

    constexpr std::uint64_t identifier() const;
    std::string string() const;

  private:
    static constexpr std::uint64_t hash_string(const std::string_view string);

  private:
    std::uint64_t hash{};
#if defined(_DEBUG)
    std::string label{};
#endif
  };
}

template <> struct std::hash<cse::name>
{
  std::size_t operator()(const cse::name &name) const;
};

#include "name.inl" // IWYU pragma: keep
