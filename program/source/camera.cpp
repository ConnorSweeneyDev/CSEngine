#include "camera.hpp"

#include <array>

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"

namespace cse::core
{
  camera::transform::property::property(const glm::vec3 &value_)
    : value(value_), velocity(glm::vec3(0.0f, 0.0f, 0.0f)), acceleration(glm::vec3(0.0f, 0.0f, 0.0f)), previous(value_),
      interpolated(value_)
  {
  }

  camera::transform::transform(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_)
    : translation(translation_), forward(forward_), up(up_)
  {
  }

  camera::graphics::graphics(const float fov_) : fov(fov_), near_clip(0.01f), far_clip(100.0f) {}

  camera::camera(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_, const float fov_)
    : transform(translation_, forward_, up_), graphics(fov_)
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

  void camera::simulate(const double simulation_alpha)
  {
    transform.translation.previous = transform.translation.value;
    transform.forward.previous = transform.forward.value;
    transform.up.previous = transform.up.value;

    if (handle_simulate) handle_simulate();

    transform.translation.interpolated =
      transform.translation.previous +
      ((transform.translation.value - transform.translation.previous) * static_cast<float>(simulation_alpha));
    transform.forward.interpolated =
      transform.forward.previous +
      ((transform.forward.value - transform.forward.previous) * static_cast<float>(simulation_alpha));
    transform.up.interpolated =
      transform.up.previous + ((transform.up.value - transform.up.previous) * static_cast<float>(simulation_alpha));
  }

  std::array<glm::mat4, 2> camera::render(const int width, const int height, const float scale_factor)
  {
    graphics.projection_matrix =
      glm::perspective(glm::radians(graphics.fov), static_cast<float>(width) / static_cast<float>(height),
                       graphics.near_clip, graphics.far_clip);
    graphics.view_matrix = glm::lookAt(
      transform.translation.interpolated * scale_factor,
      (transform.translation.interpolated * scale_factor) + transform.forward.interpolated, transform.up.interpolated);
    return {graphics.projection_matrix, graphics.view_matrix};
  }
}
