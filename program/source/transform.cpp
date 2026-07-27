#include "transform.hpp"

#include <algorithm>
#include <cmath>

#include "glm/ext/vector_double2.hpp"

#include "numeric.hpp"
#include "resource.hpp"

namespace cse::help::transform
{
  glm::dvec2 rotate(const glm::dvec2 &value, const int steps)
  {
    switch (((steps % 4) + 4) % 4)
    {
      case 1: return {value.y, -value.x};
      case 2: return {-value.x, -value.y};
      case 3: return {-value.y, value.x};
      default: return value;
    }
  }

  glm::dvec2 unrotate(const glm::dvec2 &value, const int steps)
  {
    switch (((steps % 4) + 4) % 4)
    {
      case 1: return {-value.y, value.x};
      case 2: return {-value.x, -value.y};
      case 3: return {value.y, -value.x};
      default: return value;
    }
  }

  cse::rectangle turn(const cse::rectangle &value, const int steps)
  {
    if (((steps % 4) + 4) % 4 == 0) return value;
    const auto first{rotate({value.left, value.top}, steps)};
    const auto second{rotate({value.right, value.bottom}, steps)};
    return {std::min(first.x, second.x), std::max(first.y, second.y), std::max(first.x, second.x),
            std::min(first.y, second.y)};
  }

  glm::dvec2 grid(const double extent_x, const double extent_y)
  { return {std::llround(extent_x) % 2 == 0 ? 0.5 : 0.0, std::llround(extent_y) % 2 == 0 ? -0.5 : 0.0}; }

  glm::dvec2 anchor(const int steps, const cse::flip &flip, const double scale_x, const double scale_y,
                    const unsigned int frame_width, const unsigned int frame_height, const glm::dvec2 &pivot)
  {
    const auto width{static_cast<double>(frame_width)};
    const auto height{static_cast<double>(frame_height)};
    const auto flipped_x{flip.horizontal ? width - 1.0 - pivot.x : pivot.x};
    const auto flipped_y{flip.vertical ? height - 1.0 - pivot.y : pivot.y};
    const glm::dvec2 center{(flipped_x + 0.5 - (width / 2.0)) * scale_x, (flipped_y + 0.5 - (height / 2.0)) * scale_y};
    const int turns{((steps % 4) + 4) % 4};
    const auto rotated{rotate(center, turns)};
    const auto extent_x{turns % 2 != 0 ? scale_y : scale_x};
    const auto extent_y{turns % 2 != 0 ? scale_x : scale_y};
    return grid(extent_x, extent_y) - rotated;
  }

  double snap_x(const double center, const double size)
  { return static_cast<int>(size) % 2 == 0 ? std::floor(center) + 0.5 : std::floor(center + 0.5); }

  double snap_y(const double center, const double size)
  { return static_cast<int>(size) % 2 == 0 ? std::ceil(center) - 0.5 : std::ceil(center - 0.5); }
}
