#include "camera.hpp"

#include <tuple>
#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

namespace cse
{
  camera::camera(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform_, const float fov_)
    : state(transform_), graphics(fov_)
  {
  }

  camera::~camera()
  {
    hooks.clear();
    parent.reset();
  }

  void camera::initialize()
  {
    initialized = true;
    hooks.call<void()>("initialize");
  }

  void camera::event(const SDL_Event &event) { hooks.call<void(const SDL_Event &)>("event", event); }

  void camera::input(const bool *keys) { hooks.call<void(const bool *)>("input", keys); }

  void camera::simulate()
  {
    state.translation.update();
    state.forward.update();
    state.up.update();
    hooks.call<void()>("simulate");
  }

  std::pair<glm::mat4, glm::mat4> camera::render(const double simulation_alpha, const float target_aspect_ratio,
                                                 const float global_scale_factor)
  {
    state.translation.interpolate(simulation_alpha);
    state.forward.interpolate(simulation_alpha);
    state.up.interpolate(simulation_alpha);
    auto matrices = std::pair{graphics.calculate_projection_matrix(target_aspect_ratio),
                              state.calculate_view_matrix(global_scale_factor)};
    hooks.call<void(const glm::mat4 &, const glm::mat4 &)>("render", matrices.first, matrices.second);
    return matrices;
  }

  void camera::cleanup()
  {
    initialized = false;
    hooks.call<void()>("cleanup");
  }
}
