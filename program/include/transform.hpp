#pragma once

#include "glm/ext/vector_float3.hpp"

#include "declaration.hpp"

namespace cse::help
{
  class transform_value
  {
    friend class cse::camera;
    friend class cse::object;
    friend struct camera_state;
    friend struct object_state;
    friend struct scene_graphics;

  public:
    transform_value() = default;
    transform_value(const glm::vec3 &value_);

  private:
    void update_previous();
    void interpolate(const double alpha);

  public:
    glm::vec3 value{};
    glm::vec3 velocity{};
    glm::vec3 acceleration{};

  private:
    glm::vec3 previous{};
    glm::vec3 interpolated{};
  };
}
