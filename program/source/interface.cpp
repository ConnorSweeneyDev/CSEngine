#include "interface.hpp"

#include "SDL3/SDL_events.h"

#include "exception.hpp"
#include "graphics.hpp"
#include "state.hpp"

namespace cse
{
  interface::interface(const initial_state &state_, const initial_graphics &graphics_)
    : state{state_.translation, state_.rotation, state_.scale, state_.text, state_.priority},
      graphics{graphics_.shader, graphics_.texture, graphics_.render, graphics_.text, graphics_.priority}
  {
  }

  void interface::on_prepare() {}
  void interface::prepare()
  {
    if (state.active.phase != help::phase::CLEANED)
      throw exception("Interface '{}' must be cleaned before preparation", name.string());
    state.active.phase = help::phase::PREPARED;
    on_prepare();
  }

  void interface::on_create() {}
  void interface::create()
  {
    if (state.active.phase != help::phase::PREPARED)
      throw exception("Interface '{}' must be prepared before creation", name.string());
    state.active.phase = help::phase::CREATED;
    on_create();
  }

  void interface::on_previous() {}
  void interface::previous()
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Interface '{}' must be created before updating previous state", name.string());
    state.update_previous();
    graphics.update_previous();
    on_previous();
  }

  void interface::on_event(const SDL_Event &) {}
  void interface::event(const SDL_Event &event)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Interface '{}' must be created before processing events", name.string());
    on_event(event);
  }

  void interface::on_input(const bool *) {}
  void interface::input(const bool *input)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Interface '{}' must be created before processing input", name.string());
    on_input(input);
  }

  void interface::on_simulate(const double) {}
  void interface::simulate(const double tick)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Interface '{}' must be created before simulation", name.string());
    state.active.timer.update(tick);
    graphics.animate(tick);
    on_simulate(tick);
  }

  void interface::on_render(const double) {}
  void interface::render(const double alpha)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Interface '{}' must be created before rendering", name.string());
    on_render(alpha);
  }

  void interface::on_destroy() {}
  void interface::destroy()
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Interface '{}' must be created before destruction", name.string());
    state.active.phase = help::phase::PREPARED;
    on_destroy();
  }

  void interface::on_clean() {}
  void interface::clean()
  {
    if (state.active.phase != help::phase::PREPARED)
      throw exception("Interface '{}' must be prepared before cleaning", name.string());
    state.active.phase = help::phase::CLEANED;
    on_clean();
  }
}
