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
    std::uint64_t hash{};
#if defined(_DEBUG)
    std::string label{};
#endif
  };

  class identity : public cse::name
  {
    friend class game;
    friend class scene;

  public:
    identity() = default;
    ~identity() = default;
    identity(const identity &) = default;
    identity &operator=(const identity &) = delete;
    identity(identity &&) = delete;
    identity &operator=(identity &&) = delete;

  private:
    identity &operator=(const cse::name &other);
  };
}

template <> struct std::hash<cse::name>
{
  std::size_t operator()(const cse::name &name) const;
};

#include "name.inl" // IWYU pragma: keep
