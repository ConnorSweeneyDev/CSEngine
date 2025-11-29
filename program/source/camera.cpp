#include "camera.hpp"

#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

namespace cse::core
{
  camera::camera(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_, const float fov_)
    : transform(translation_, forward_, up_), graphics(fov_)
  {
  }

  camera::~camera()
  {
    handle_simulate = nullptr;
    handle_input = nullptr;
  }

  void camera::event(const SDL_Event &event)
  {
    if (handle_event) handle_event(event);
  }

  void camera::input(const bool *keys)
  {
    if (handle_input) handle_input(keys);
  }

  void camera::simulate(const double simulation_alpha)
  {
    transform.translation.update();
    transform.forward.update();
    transform.up.update();

    if (handle_simulate) handle_simulate();

    transform.translation.interpolate(simulation_alpha);
    transform.forward.interpolate(simulation_alpha);
    transform.up.interpolate(simulation_alpha);
  }

  std::pair<glm::mat4, glm::mat4> camera::render(const float target_aspect_ratio, const float global_scale_factor)
  {
    return {graphics.calculate_projection_matrix(target_aspect_ratio),
            graphics.calculate_view_matrix(transform.translation.interpolated, transform.forward.interpolated,
                                           transform.up.interpolated, global_scale_factor)};
  }
}
