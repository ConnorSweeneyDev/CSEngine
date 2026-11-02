#pragma once

#include <algorithm>
#include <concepts>
#include <type_traits>
#include <utility>

#include "temporal.hpp"

namespace cse
{
  namespace ease
  {
    constexpr double linear(const double progress) { return progress; }
    constexpr double in(const double progress) { return progress * progress; }
    constexpr double out(const double progress) { return 1.0 - ((1.0 - progress) * (1.0 - progress)); }
    constexpr double in_out(const double progress) { return progress * progress * (3.0 - (2.0 * progress)); }
  }

  template <typename type> template <typename... arguments>
    requires std::constructible_from<type, arguments &&...>
  temporal<type>::temporal(arguments &&...arguments_) : value(std::forward<arguments>(arguments_)...)
  {
  }

  template <typename type> type temporal<type>::interpolated(const temporal &previous, const double alpha) const
  {
    if (!interpolate || instant) return value;
    return previous.value + ((value - previous.value) * alpha);
  }

  template <typename type> void temporal<type>::synchronize() { instant = false; }

  template <typename type> void temporal<type>::simulate(const double tick)
  {
    const auto step{[tick](cse::motion &mot, double &val)
                    {
                      if (!mot.moving) return;
                      const auto duration{std::max(mot.duration, 0.0)};
                      mot.elapsed = std::clamp(mot.elapsed + (tick * mot.speed), 0.0, duration);
                      auto progress{mot.speed < 0.0 ? 0.0 : 1.0};
                      if (duration > 0.0) progress = mot.elapsed / duration;
                      const auto eased{mot.ease ? mot.ease(progress) : progress};
                      val = mot.from + ((mot.to - mot.from) * eased);
                      if ((mot.speed > 0.0 && mot.elapsed >= duration) || (mot.speed < 0.0 && mot.elapsed <= 0.0))
                        mot.moving = false;
                    }};
    if constexpr (std::is_same_v<typename help::axes<type>::motion, cse::motion>)
      step(motion, value);
    else
    {
      step(motion.x, value.x);
      step(motion.y, value.y);
      if constexpr (type::length() > 2) step(motion.z, value.z);
      if constexpr (type::length() > 3) step(motion.w, value.w);
    }
  }
}
