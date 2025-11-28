#include "transform.hpp"

#include "glm/ext/vector_float3.hpp"

namespace cse::helper
{
  transform_value::transform_value(const glm::vec3 &value_) : value(value_), previous(value_), interpolated(value_) {}

  void transform_value::interpolate(const double alpha)
  {
    interpolated = previous + ((value - previous) * static_cast<float>(alpha));
  }

  void transform_value::update() { previous = value; }

  camera_transform::camera_transform(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_)
    : translation(translation_), forward(forward_), up(up_)
  {
  }

  object_transform::object_transform(const glm::vec3 &translation_, const glm::vec3 &rotation_, const glm::vec3 &scale_)
    : translation(translation_), rotation(rotation_), scale(scale_)
  {
  }
}
