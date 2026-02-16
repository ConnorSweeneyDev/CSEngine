#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

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

    bool operator==(const name &other) const;

    constexpr std::uint64_t identifier() const;
    std::string string() const;

  private:
    static constexpr std::uint64_t hash_compiletime(const char *string, std::uint64_t hash = 14695981039346656037ULL);
    static std::uint64_t hash_runtime(const std::string &string);

  private:
    std::uint64_t hash{};
#if defined(_DEBUG)
    std::string label{};
#endif
  };
}

template <> struct std::hash<cse::name>
{
  std::size_t operator()(const cse::name &id) const;
};

#include "name.inl" // IWYU pragma: keep
