#include "object.hpp"

#include "SDL3/SDL_events.h"

#include "exception.hpp"
#include "graphics.hpp"
#include "state.hpp"

namespace cse
{
  object::object(const initial_state &state_, const initial_graphics &graphics_)
    : state{state_.translation, state_.rotation, state_.scale, state_.collidable, state_.priority},
      graphics{graphics_.shader, graphics_.texture, graphics_.priority}
  {
  }

  void object::on_prepare() {}
  void object::prepare()
  {
    if (state.active.phase != help::phase::CLEANED)
      throw exception("Object '{}' must be cleaned before preparation", name.string());
    state.active.phase = help::phase::PREPARED;
    on_prepare();
  }

  void object::on_create() {}
  void object::create()
  {
    if (state.active.phase != help::phase::PREPARED)
      throw exception("Object '{}' must be prepared before creation", name.string());
    state.active.phase = help::phase::CREATED;
    on_create();
  }

  void object::on_synchronize() {}
  void object::synchronize()
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Object '{}' must be created before synchronization", name.string());
    state.synchronize();
    graphics.synchronize();
    on_synchronize();
  }

  void object::on_event(const SDL_Event &) {}
  void object::event(const SDL_Event &event)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Object '{}' must be created before processing events", name.string());
    on_event(event);
  }

  void object::on_simulate(const double) {}
  void object::simulate(const double tick)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Object '{}' must be created before simulation", name.string());
    state.active.timer.update(tick);
    graphics.animate(tick);
    on_simulate(tick);
  }

  void object::on_collide(const double) {}
  void object::collide(const double tick)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Object '{}' must be created before simulation", name.string());
    on_collide(tick);
  }

  void object::on_destroy() {}
  void object::destroy()
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Object '{}' must be created before destruction", name.string());
    state.active.phase = help::phase::PREPARED;
    on_destroy();
  }

  void object::on_clean() {}
  void object::clean()
  {
    if (state.active.phase != help::phase::PREPARED)
      throw exception("Object '{}' must be prepared before cleaning", name.string());
    state.active.phase = help::phase::CLEANED;
    on_clean();
  }
}
