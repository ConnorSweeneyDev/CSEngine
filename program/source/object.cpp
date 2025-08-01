#include "object.hpp"

#include "glm/ext/vector_float3.hpp"

#include "resource.hpp"

namespace cse::base
{
  object::transform::property::property(const glm::vec3 &starting_value)
    : current(starting_value), previous(starting_value), interpolated(starting_value),
      velocity(glm::vec3(0.0f, 0.0f, 0.0f)), acceleration(glm::vec3(0.0f, 0.0f, 0.0f))
  {
  }

  object::transform::transform(const glm::vec3 &starting_translation, const glm::vec3 &starting_rotation,
                               const glm::vec3 &starting_scale)
    : translation(starting_translation), rotation(starting_rotation), scale(starting_scale)
  {
  }

  object::graphics::graphics(const resource::compiled_shader &vertex_shader,
                             const resource::compiled_shader &fragment_shader)
    : shader(vertex_shader, fragment_shader)
  {
  }

  object::object(const glm::vec3 &starting_translation, const glm::vec3 &starting_rotation,
                 const glm::vec3 &starting_scale, const resource::compiled_shader &vertex_shader,
                 const resource::compiled_shader &fragment_shader)
    : transform(starting_translation, starting_rotation, starting_scale), graphics(vertex_shader, fragment_shader)
  {
  }

  object::~object()
  {
    graphics.index_buffer = nullptr;
    graphics.vertex_buffer = nullptr;
    graphics.pipeline = nullptr;
  }
}
