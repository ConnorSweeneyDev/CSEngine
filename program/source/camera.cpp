#include "camera.hpp"

#include <tuple>
#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

namespace cse
{
  camera::camera(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform_, const double fov_)
    : state{transform_}, graphics{fov_}
  {
  }

  camera::~camera() { hook.reset(); }

  void camera::initialize()
  {
    state.initialized = true;
    hook.call<void()>("initialize");
  }

  void camera::previous()
  {
    state.update_previous();
    graphics.update_previous();
  }

  void camera::event(const SDL_Event &event) { hook.call<void(const SDL_Event &)>("event", event); }

  void camera::input(const bool *keys) { hook.call<void(const bool *)>("input", keys); }

  void camera::simulate(const float poll_rate) { hook.call<void(const float)>("simulate", poll_rate); }

  std::pair<glm::mat4, glm::mat4> camera::render(const double alpha, const float aspect_ratio)
  {
    hook.call<void(const double)>("render", alpha);
    return {graphics.calculate_projection_matrix(alpha, aspect_ratio), state.calculate_view_matrix(alpha)};
  }

  void camera::cleanup()
  {
    state.initialized = false;
    hook.call<void()>("cleanup");
  }
}
