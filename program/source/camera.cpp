#include "camera.hpp"

#include <array>

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"

namespace cse::core
{
  camera::graphics::graphics(const float fov_) : fov(fov_), near_clip(0.01f), far_clip(100.0f) {}

  glm::mat4 camera::graphics::calculate_projection_matrix(const unsigned int width, const unsigned int height)
  {
    projection_matrix =
      glm::perspective(glm::radians(fov), static_cast<float>(width) / static_cast<float>(height), near_clip, far_clip);
    return projection_matrix;
  }

  glm::mat4 camera::graphics::calculate_view_matrix(const glm::vec3 &translation, const glm::vec3 &forward,
                                                    const glm::vec3 &up, const float scale_factor)
  {
    view_matrix = glm::lookAt(translation * scale_factor, (translation * scale_factor) + forward, up);
    return view_matrix;
  }

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

    transform.translation.interpolate(simulation_alpha);
    transform.forward.interpolate(simulation_alpha);
    transform.up.interpolate(simulation_alpha);
  }

  std::array<glm::mat4, 2> camera::render(const unsigned int width, const unsigned int height, const float scale_factor)
  {
    return {graphics.calculate_projection_matrix(width, height),
            graphics.calculate_view_matrix(transform.translation.interpolated, transform.forward.interpolated,
                                           transform.up.interpolated, scale_factor)};
  }
}
