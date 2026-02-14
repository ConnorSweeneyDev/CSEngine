#include "camera.hpp"

#include <tuple>
#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_double3.hpp"

#include "exception.hpp"
#include "state.hpp"

namespace cse
{
  camera::camera(const std::tuple<glm::dvec3, glm::dvec3, glm::dvec3> &transform_, const double fov_)
    : state{transform_}, graphics{fov_}
  {
  }

  void camera::prepare()
  {
    if (state.active.phase != help::phase::CLEANED) throw exception("Camera must be cleaned before preparation");
    state.active.phase = help::phase::PREPARED;
    hooks.call<void()>(hook::PREPARE);
  }

  void camera::create()
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Camera must be prepared before creation");
    state.active.phase = help::phase::CREATED;
    hooks.call<void()>(hook::CREATE);
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
    hooks.call<void(const SDL_Event &)>(hook::EVENT, event);
  }

  void camera::input(const bool *input)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Camera must be created before processing input");
    hooks.call<void(const bool *)>(hook::INPUT, input);
  }

  void camera::simulate(const double poll_rate)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Camera must be created before simulation");
    timers.update(poll_rate);
    hooks.call<void(const double)>(hook::SIMULATE, poll_rate);
  }

  std::pair<glm::dmat4, glm::dmat4> camera::render(const double alpha, const double previous_aspect_ratio,
                                                   const double active_aspect_ratio)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Camera must be created before rendering");
    hooks.call<void(const double)>(hook::RENDER, alpha);
    return {graphics.calculate_projection_matrix(alpha, previous_aspect_ratio, active_aspect_ratio),
            state.calculate_view_matrix(alpha)};
  }

  void camera::destroy()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Camera must be created before destruction");
    state.active.phase = help::phase::PREPARED;
    hooks.call<void()>(hook::DESTROY);
  }

  void camera::clean()
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Camera must be prepared before cleaning");
    state.active.phase = help::phase::CLEANED;
    hooks.call<void()>(hook::CLEAN);
  }
}
