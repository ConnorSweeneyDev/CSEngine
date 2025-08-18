#include "camera.hpp"

#include <array>

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
    switch (event.type)
    {
      case SDL_EVENT_KEY_DOWN:
        if (handle_event) handle_event(event.key);
        break;
      default: break;
    }
  }

  void camera::input(const bool *keys)
  {
    if (handle_input) handle_input(keys);
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
