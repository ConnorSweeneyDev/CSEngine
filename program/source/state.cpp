#include "state.hpp"

#include "glm/ext/vector_float3.hpp"

namespace cse::helper
{
  camera_state::camera_state(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_)
    : translation(translation_), forward(forward_), up(up_)
  {
  }

  object_state::object_state(const glm::vec3 &translation_, const glm::vec3 &rotation_, const glm::vec3 &scale_)
    : translation(translation_), rotation(rotation_), scale(scale_)
  {
  }
}
