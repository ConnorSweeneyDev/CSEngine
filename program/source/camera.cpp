#include "camera.hpp"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"

namespace cse::base
{
  camera::transform::property::property(const glm::vec3 &value_)
    : current(value_), previous(value_), interpolated(value_), velocity(glm::vec3(0.0f, 0.0f, 0.0f)),
      acceleration(glm::vec3(0.0f, 0.0f, 0.0f))
  {
  }

  camera::transform::transform(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_)
    : translation(translation_), forward(forward_), up(up_)
  {
  }

  camera::camera(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_, const float fov_,
                 const float near_clip_, const float far_clip_)
    : fov(fov_), near_clip(near_clip_), far_clip(far_clip_), transform(translation_, forward_, up_), graphics()
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

  struct camera::graphics camera::get_graphics() const { return graphics; }
}
