#include "camera.hpp"

#include <tuple>
#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

#include "exception.hpp"
#include "state.hpp"

namespace cse
{
  camera::camera(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform_, const double fov_)
    : state{transform_}, graphics{fov_}
  {
  }

  camera::~camera() { hook.reset(); }

  void camera::prepare()
  {
    if (state.active.phase != help::phase::CLEANED) throw exception("Camera must be cleaned before preparation");
    state.active.phase = help::phase::PREPARED;
    hook.call<void()>(hooks::PREPARE());
  }

  void camera::create()
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Camera must be prepared before creation");
    state.active.phase = help::phase::CREATED;
    hook.call<void()>(hooks::CREATE());
  }

  void camera::previous()
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Camera must be created before updating previous state");
    state.update_previous();
    graphics.update_previous();
  }

  void camera::event(const SDL_Event &event)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Camera must be created before processing events");
    hook.call<void(const SDL_Event &)>(hooks::EVENT(), event);
  }

  void camera::input(const bool *input)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Camera must be created before processing input");
    hook.call<void(const bool *)>(hooks::INPUT(), input);
  }

  void camera::simulate(const float poll_rate)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Camera must be created before simulation");
    hook.call<void(const float)>(hooks::SIMULATE(), poll_rate);
  }

  std::pair<glm::mat4, glm::mat4> camera::render(const double alpha, const float aspect_ratio)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Camera must be created before rendering");
    hook.call<void(const double)>(hooks::RENDER(), alpha);
    return {graphics.calculate_projection_matrix(alpha, aspect_ratio), state.calculate_view_matrix(alpha)};
  }

  void camera::destroy()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Camera must be created before destruction");
    state.active.phase = help::phase::PREPARED;
    hook.call<void()>(hooks::DESTROY());
  }

  void camera::clean()
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Camera must be prepared before cleaning");
    state.active.phase = help::phase::CLEANED;
    hook.call<void()>(hooks::CLEAN());
  }
}
