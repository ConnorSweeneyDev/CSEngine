#include "camera.hpp"

#include <array>

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

  void camera::transform::property::update_previous() { previous = value; }

  void camera::transform::property::update_interpolated(float simulation_alpha)
  {
    interpolated = previous + ((value - previous) * simulation_alpha);
  }

  camera::transform::transform(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_)
    : translation(translation_), forward(forward_), up(up_)
  {
  }

  camera::graphics::graphics(float fov_, float near_clip_, float far_clip_)
    : fov(fov_), near_clip(near_clip_), far_clip(far_clip_)
  {
  }

  void camera::graphics::update_projection_matrix(int width, int height)
  {
    projection_matrix =
      glm::perspective(glm::radians(fov), static_cast<float>(width) / static_cast<float>(height), near_clip, far_clip);
  }

  void camera::graphics::update_view_matrix(const glm::vec3 &translation, const glm::vec3 &forward, const glm::vec3 &up)
  {
    view_matrix = glm::lookAt(translation, translation + forward, up);
  }

  camera::camera(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_, float fov_,
                 float near_clip_, float far_clip_)
    : transform(translation_, forward_, up_), graphics(fov_, near_clip_, far_clip_)
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

  std::array<glm::mat4, 2> camera::render(int width, int height)
  {
    graphics.update_projection_matrix(width, height);
    graphics.update_view_matrix(transform.translation.interpolated, transform.forward.interpolated,
                                transform.up.interpolated);
    return {graphics.projection_matrix, graphics.view_matrix};
  }
}
