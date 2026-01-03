#include "camera.hpp"

#include <tuple>
#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

#include "transform.hpp"

namespace cse
{
  camera::camera(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform_, const float fov_)
    : state{transform_}, graphics{fov_}, previous{state, graphics}
  {
  }

  camera::~camera()
  {
    hook.reset();
    parent.reset();
  }

  void camera::initialize()
  {
    initialized = true;
    hook.call<void()>("initialize");
  }

  void camera::event(const SDL_Event &event) { hook.call<void(const SDL_Event &)>("event", event); }

  void camera::input(const bool *keys) { hook.call<void(const bool *)>("input", keys); }

  void camera::simulate(const double active_poll_rate)
  {
    state.translation.update();
    state.forward.update();
    state.up.update();
    hook.call<void(const float)>("simulate", static_cast<float>(active_poll_rate));
  }

  std::pair<glm::mat4, glm::mat4> camera::render(const float aspect_ratio, const float scale_factor)
  {
    auto matrices =
      std::pair{graphics.calculate_projection_matrix(aspect_ratio), state.calculate_view_matrix(scale_factor)};
    hook.call<void(const glm::mat4 &, const glm::mat4 &)>("render", matrices.first, matrices.second);
    return matrices;
  }

  void camera::cleanup()
  {
    initialized = false;
    hook.call<void()>("cleanup");
  }

  void camera::update_previous() { previous.update(state, graphics); }
}
