#include "camera.hpp"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"

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

  camera::~camera()
  {
    handle_simulate = nullptr;
    handle_input = nullptr;
  }

  void camera::input(const bool *key_state)
  {
    if (handle_input) handle_input(key_state);
  }

  void camera::simulate(double simulation_alpha)
  {
    if (handle_simulate) handle_simulate(simulation_alpha);
  }

  void camera::render(int width, int height)
  {
    graphics.projection_matrix =
      glm::perspective(glm::radians(fov), static_cast<float>(width) / static_cast<float>(height), near_clip, far_clip);
    graphics.view_matrix =
      glm::lookAt(transform.translation.interpolated,
                  transform.translation.interpolated + transform.forward.interpolated, transform.up.interpolated);
  }
}
