#include "camera.hpp"

#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

namespace cse::core
{
  camera::camera(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_, const float fov_)
    : state(translation_, forward_, up_), graphics(fov_)
  {
  }

  camera::~camera()
  {
    simulate_hooks.clear();
    input_hooks.clear();
    event_hooks.clear();
  }

  void camera::event(const SDL_Event &event) { event_hooks.call("main", event); }

  void camera::input(const bool *keys) { input_hooks.call("main", keys); }

  void camera::simulate(const double simulation_alpha)
  {
    state.translation.update();
    state.forward.update();
    state.up.update();
    simulate_hooks.call("main");
    state.translation.interpolate(simulation_alpha);
    state.forward.interpolate(simulation_alpha);
    state.up.interpolate(simulation_alpha);
  }

  std::pair<glm::mat4, glm::mat4> camera::render(const float target_aspect_ratio, const float global_scale_factor)
  {
    return {graphics.calculate_projection_matrix(target_aspect_ratio),
            state.calculate_view_matrix(global_scale_factor)};
  }
}
