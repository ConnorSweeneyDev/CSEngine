#include "camera.hpp"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"

namespace cse::base
{
  camera::transform::property::property(const glm::vec3 &value_)
    : value(value_), velocity(glm::vec3(0.0f, 0.0f, 0.0f)), acceleration(glm::vec3(0.0f, 0.0f, 0.0f)), previous(value_),
      interpolated(value_)
  {
  }

  glm::vec3 camera::transform::property::get_previous() const { return previous; }

  glm::vec3 camera::transform::property::get_interpolated() const { return interpolated; }

  void camera::transform::property::update_previous() { previous = value; }

  void camera::transform::property::update_interpolated(float simulation_alpha)
  {
    interpolated = previous + ((value - previous) * simulation_alpha);
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
    transform.translation.update_previous();
    transform.forward.update_previous();
    transform.up.update_previous();

    if (handle_simulate) handle_simulate();

    transform.translation.update_interpolated(static_cast<float>(simulation_alpha));
    transform.forward.update_interpolated(static_cast<float>(simulation_alpha));
    transform.up.update_interpolated(static_cast<float>(simulation_alpha));
  }

  void camera::render(int width, int height)
  {
    graphics.projection_matrix =
      glm::perspective(glm::radians(fov), static_cast<float>(width) / static_cast<float>(height), near_clip, far_clip);
    graphics.view_matrix = glm::lookAt(transform.translation.get_interpolated(),
                                       transform.translation.get_interpolated() + transform.forward.get_interpolated(),
                                       transform.up.get_interpolated());
  }

  struct camera::graphics camera::get_graphics() const { return graphics; }
}
