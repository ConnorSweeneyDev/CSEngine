#pragma once

#include "glm/ext/vector_double2.hpp"

#include "numeric.hpp"
#include "resource.hpp"

namespace cse::help::transform
{
  glm::dvec2 rotate(const glm::dvec2 &value, const int steps);
  glm::dvec2 unrotate(const glm::dvec2 &value, const int steps);
  cse::rectangle turn(const cse::rectangle &value, const int steps);
  glm::dvec2 grid(const double extent_x, const double extent_y);
  glm::dvec2 anchor(const int steps, const cse::flip &flip, const double scale_x, const double scale_y,
                    const unsigned int frame_width, const unsigned int frame_height, const glm::dvec2 &pivot);
  double snap_x(const double center, const double size);
  double snap_y(const double center, const double size);
}
