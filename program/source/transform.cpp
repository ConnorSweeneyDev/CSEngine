#include "transform.hpp"

#include "glm/ext/vector_float3.hpp"

namespace cse::help
{
  transform_value::transform_value(const glm::vec3 &value_) : value{value_}, previous{value_}, interpolated{value_} {}

  void transform_value::interpolate(const double alpha)
  {
    interpolated = previous + ((value - previous) * static_cast<float>(alpha));
  }

  void transform_value::update() { previous = value; }
}
