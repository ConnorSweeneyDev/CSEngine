#include "light.hpp"

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/ext/vector_double4.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"

#include "core.hpp"
#include "exception.hpp"
#include "temporal.hpp"

namespace cse::help::light
{
  previous::previous(const temporal<glm::dvec3> &translation_, const temporal<glm::dvec2> &rotation_,
                     const light::illumination &illumination_, const light::shadow &shadow_, const int priority_)
    : translation{translation_}, rotation{rotation_}, illumination{illumination_}, shadow{shadow_}, priority{priority_}
  {
  }

  active::active(const temporal<glm::dvec3> &translation_, const temporal<glm::dvec2> &rotation_,
                 const light::illumination &illumination_, const light::shadow &shadow_, const int priority_)
    : translation{translation_}, rotation{rotation_}, illumination{illumination_}, shadow{shadow_}, priority{priority_}
  {
  }

  void active::synchronize(previous &last)
  {
    last.translation = translation;
    last.rotation = rotation;
    last.illumination = illumination;
    last.shadow = shadow;
    last.priority = priority;

    last.timer = timer;
    last.mixer = mixer;
    last.phase = phase;

    for (auto &[name, sound] : mixer.sounds)
    {
      sound.speed.instant = false;
      sound.volume.instant = false;
    }
    for (auto &[name, music] : mixer.musics)
    {
      music.speed.instant = false;
      music.volume.instant = false;
    }
    translation.instant = false;
    rotation.instant = false;
    illumination.brightness.instant = false;
    illumination.penetration.instant = false;
    illumination.softness.instant = false;
    illumination.range.instant = false;
    illumination.angle.instant = false;
    shadow.darkness.instant = false;
    shadow.softness.instant = false;
  }

  glm::dvec3 active::calculate_direction(const previous &last, const double alpha) const
  {
    const auto interpolated_rotation{rotation.interpolated(last.rotation, alpha)};
    auto orientation{glm::dmat4(1.0)};
    orientation = glm::rotate(orientation, glm::radians(-interpolated_rotation.y), {0.0, 1.0, 0.0});
    orientation = glm::rotate(orientation, glm::radians(interpolated_rotation.x), {1.0, 0.0, 0.0});
    const auto direction{glm::dvec3{orientation * glm::dvec4{0.0, 0.0, -1.0, 0.0}}};
    const auto length{glm::length(direction)};
    return length > 0.0 ? direction / length : glm::dvec3{0.0, 0.0, -1.0};
  }
}

namespace cse
{
  light::light(const initial &initial_)
    : previous{initial_.translation, initial_.rotation, initial_.illumination, initial_.shadow, initial_.priority},
      active{initial_.translation, initial_.rotation, initial_.illumination, initial_.shadow, initial_.priority}
  {
  }

  void light::on_prepare() {}
  void light::prepare()
  {
    if (active.phase != help::phase::CLEANED)
      throw exception("Light '{}' must be cleaned before preparation", name.string());
    active.phase = help::phase::PREPARED;
    on_prepare();
  }

  void light::on_create() {}
  void light::create()
  {
    if (active.phase != help::phase::PREPARED)
      throw exception("Light '{}' must be prepared before creation", name.string());
    active.phase = help::phase::CREATED;
    on_create();
  }

  void light::on_synchronize() {}
  void light::synchronize()
  {
    if (active.phase != help::phase::CREATED)
      throw exception("Light '{}' must be created before synchronization", name.string());
    active.synchronize(previous);
    on_synchronize();
  }

  void light::on_event(const SDL_Event &) {}
  void light::event(const SDL_Event &event)
  {
    if (active.phase != help::phase::CREATED)
      throw exception("Light '{}' must be created before processing events", name.string());
    on_event(event);
  }

  void light::on_simulate(const double) {}
  void light::simulate(const double tick)
  {
    if (active.phase != help::phase::CREATED)
      throw exception("Light '{}' must be created before simulation", name.string());
    active.timer.update(tick);
    on_simulate(tick);
  }

  void light::on_destroy() {}
  void light::destroy()
  {
    if (active.phase != help::phase::CREATED)
      throw exception("Light '{}' must be created before destruction", name.string());
    active.phase = help::phase::PREPARED;
    on_destroy();
  }

  void light::on_clean() {}
  void light::clean()
  {
    if (active.phase != help::phase::PREPARED)
      throw exception("Light '{}' must be prepared before cleaning", name.string());
    active.phase = help::phase::CLEANED;
    on_clean();
  }
}
