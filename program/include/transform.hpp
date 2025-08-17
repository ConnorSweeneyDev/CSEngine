#pragma once

#include "glm/ext/vector_float3.hpp"

#include "declaration.hpp"

namespace cse::helper
{
  class transform_value
  {
    friend class core::camera;
    friend class core::object;

  public:
    transform_value() = default;
    transform_value(const glm::vec3 &value_);

  private:
    void interpolate(const double alpha);

  public:
    glm::vec3 value = {};
    glm::vec3 velocity = {};
    glm::vec3 acceleration = {};

  private:
    glm::vec3 previous = {};
    glm::vec3 interpolated = {};
  };

  struct camera_transform
  {
    camera_transform() = default;
    camera_transform(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_);

    transform_value translation = {};
    transform_value forward = {};
    transform_value up = {};
  };

  struct object_transform
  {
    object_transform() = default;
    object_transform(const glm::vec3 &translation_, const glm::vec3 &rotation_, const glm::vec3 &scale_);

    transform_value translation = {};
    transform_value rotation = {};
    transform_value scale = {};
  };
}
