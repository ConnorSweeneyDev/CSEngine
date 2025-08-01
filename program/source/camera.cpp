#include "camera.hpp"

#include "glm/ext/vector_float3.hpp"

namespace cse::base
{
  camera::transform::property::property(const glm::vec3 &starting_value)
    : current(starting_value), previous(starting_value), interpolated(starting_value),
      velocity(glm::vec3(0.0f, 0.0f, 0.0f)), acceleration(glm::vec3(0.0f, 0.0f, 0.0f))
  {
  }

  camera::transform::transform(const glm::vec3 &starting_translation, const glm::vec3 &starting_forward,
                               const glm::vec3 &starting_up)
    : translation(starting_translation), forward(starting_forward), up(starting_up)
  {
  }

  camera::camera(const glm::vec3 &starting_translation, const glm::vec3 &starting_forward, const glm::vec3 &starting_up,
                 const float starting_fov, const float starting_near_clip, const float starting_far_clip)
    : graphics(), fov(starting_fov), near_clip(starting_near_clip), far_clip(starting_far_clip),
      transform(starting_translation, starting_forward, starting_up)
  {
  }

  camera::~camera() {}
}
