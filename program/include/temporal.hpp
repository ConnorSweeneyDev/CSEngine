#pragma once

#include <concepts>

#include "glm/detail/qualifier.hpp"

namespace cse
{
  namespace ease
  {
    constexpr double linear(const double progress);
    constexpr double in(const double progress);
    constexpr double out(const double progress);
    constexpr double in_out(const double progress);
  }

  struct motion
  {
    double from{};
    double to{};
    double duration{};
    double elapsed{};
    bool moving{};
    double speed{1.0};
    double (*ease)(double){cse::ease::linear};
  };

  namespace help
  {
    template <typename type> struct axes { using motion = cse::motion; };
    template <typename component, glm::qualifier precision> struct axes<glm::vec<2, component, precision>>
    {
      struct motion
      {
        cse::motion x{};
        cse::motion y{};
      };
    };
    template <typename component, glm::qualifier precision> struct axes<glm::vec<3, component, precision>>
    {
      struct motion
      {
        cse::motion x{};
        cse::motion y{};
        cse::motion z{};
      };
    };
    template <typename component, glm::qualifier precision> struct axes<glm::vec<4, component, precision>>
    {
      struct motion
      {
        cse::motion x{};
        cse::motion y{};
        cse::motion z{};
        cse::motion w{};
      };
    };
  }
}

namespace cse
{
  template <typename type> class temporal
  {
  public:
    temporal() = default;
    template <typename... arguments>
      requires std::constructible_from<type, arguments &&...>
    temporal(arguments &&...arguments_);

    void synchronize();
    void simulate(const double tick);

    type interpolated(const temporal &previous, const double alpha) const;

  public:
    type value{};
    typename help::axes<type>::motion motion{};
    bool interpolate{true};
    bool instant{};
  };
}

#include "temporal.inl" // IWYU pragma: export
