#pragma once

#include "glm/ext/vector_float3.hpp"

namespace cse::core
{
  class camera;
  class object;
}

namespace cse::helper
{
  struct transform_value
  {
    friend class core::camera;
    friend class core::object;

  public:
    transform_value(const glm::vec3 &value_);

  private:
    void interpolate(const double alpha);

  public:
    glm::vec3 value = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 acceleration = glm::vec3(0.0f, 0.0f, 0.0f);

  private:
    glm::vec3 previous = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 interpolated = glm::vec3(0.0f, 0.0f, 0.0f);
  };

  struct camera_transform
  {
  public:
    camera_transform(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_);

  public:
    transform_value translation = {glm::vec3(0.0f, 0.0f, 0.0f)};
    transform_value forward = {glm::vec3(0.0f, 0.0f, 0.0f)};
    transform_value up = {glm::vec3(0.0f, 0.0f, 0.0f)};
  };

  struct object_transform
  {
  public:
    object_transform(const glm::vec3 &translation_, const glm::vec3 &rotation_, const glm::vec3 &scale_);

  public:
    transform_value translation = {glm::vec3(0.0f, 0.0f, 0.0f)};
    transform_value rotation = {glm::vec3(0.0f, 0.0f, 0.0f)};
    transform_value scale = {glm::vec3(0.0f, 0.0f, 0.0f)};
  };
}
