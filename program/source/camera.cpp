#include "camera.hpp"

#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_double4x4.hpp"

#include "exception.hpp"
#include "state.hpp"

namespace cse
{
  camera::camera(const initial_state &state_, const initial_graphics &graphics_)
    : state{state_.translation, state_.forward, state_.up}, graphics{graphics_.fov, graphics_.clip}
  {
  }

  void camera::on_prepare() {}
  void camera::prepare()
  {
    if (state.active.phase != help::phase::CLEANED) throw exception("Camera must be cleaned before preparation");
    state.active.phase = help::phase::PREPARED;
    on_prepare();
  }

  void camera::on_create() {}
  void camera::create()
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Camera must be prepared before creation");
    state.active.phase = help::phase::CREATED;
    on_create();
  }

  void camera::on_previous() {}
  void camera::previous()
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Camera must be created before updating previous state");
    state.update_previous();
    graphics.update_previous();
    on_previous();
  }

  void camera::on_event(const SDL_Event &) {}
  void camera::event(const SDL_Event &event)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Camera must be created before processing events");
    on_event(event);
  }

  void camera::on_simulate(const double) {}
  void camera::simulate(const double tick)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Camera must be created before simulation");
    state.active.timer.update(tick);
    on_simulate(tick);
  }

  void camera::on_render(const double) {}
  std::pair<glm::dmat4, glm::dmat4> camera::render(const double previous_aspect, const double active_aspect,
                                                   const double alpha)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Camera must be created before rendering");
    on_render(alpha);
    return {graphics.calculate_projection_matrix(previous_aspect, active_aspect, alpha),
            state.calculate_view_matrix(alpha)};
  }

  void camera::on_destroy() {}
  void camera::destroy()
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Camera must be created before destruction");
    state.active.phase = help::phase::PREPARED;
    on_destroy();
  }

  void camera::on_clean() {}
  void camera::clean()
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Camera must be prepared before cleaning");
    state.active.phase = help::phase::CLEANED;
    on_clean();
  }
}
