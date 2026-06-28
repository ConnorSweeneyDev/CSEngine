#include "camera.hpp"

#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/trigonometric.hpp"

#include "core.hpp"
#include "exception.hpp"
#include "temporal.hpp"

namespace cse::help::camera
{
  previous::previous(const temporal<glm::dvec3> &translation_, const temporal<glm::dvec3> &forward_,
                     const temporal<glm::dvec3> &up_, const temporal<double> &fov_, const camera::clip &clip_)
    : translation{translation_}, forward{forward_}, up{up_}, fov{fov_}, clip{clip_}
  {
  }

  active::active(const temporal<glm::dvec3> &translation_, const temporal<glm::dvec3> &forward_,
                 const temporal<glm::dvec3> &up_, const temporal<double> &fov_, const camera::clip &clip_)
    : translation{translation_}, forward{forward_}, up{up_}, fov{fov_}, clip{clip_}
  {
  }

  void active::synchronize(previous &last)
  {
    last.translation = translation;
    last.forward = forward;
    last.up = up;
    last.fov = fov;
    last.clip = clip;
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
    forward.instant = false;
    up.instant = false;
    fov.instant = false;
  }

  glm::dmat4 active::calculate_view_matrix(const previous &last, const double alpha) const
  {
    auto interpolated_translation = translation.interpolated(last.translation, alpha);
    return glm::lookAt(interpolated_translation, interpolated_translation + forward.interpolated(last.forward, alpha),
                       up.interpolated(last.up, alpha));
  }

  glm::dmat4 active::calculate_projection_matrix(const previous &last, const double aspect, const double alpha)
  { return glm::perspectiveRH_ZO(glm::radians(fov.interpolated(last.fov, alpha)), aspect, clip.near, clip.far); }
}

namespace cse
{
  camera::camera(const initial &initial_)
    : previous{initial_.translation, initial_.forward, initial_.up, initial_.fov, initial_.clip},
      active{initial_.translation, initial_.forward, initial_.up, initial_.fov, initial_.clip}
  {
  }

  void camera::on_prepare() {}
  void camera::prepare()
  {
    if (active.phase != help::phase::CLEANED) throw exception("Camera must be cleaned before preparation");
    active.phase = help::phase::PREPARED;
    on_prepare();
  }

  void camera::on_create() {}
  void camera::create()
  {
    if (active.phase != help::phase::PREPARED) throw exception("Camera must be prepared before creation");
    active.phase = help::phase::CREATED;
    on_create();
  }

  void camera::on_synchronize() {}
  void camera::synchronize()
  {
    if (active.phase != help::phase::CREATED) throw exception("Camera must be created before synchronization");
    active.synchronize(previous);
    on_synchronize();
  }

  void camera::on_event(const SDL_Event &) {}
  void camera::event(const SDL_Event &event)
  {
    if (active.phase != help::phase::CREATED) throw exception("Camera must be created before processing events");
    on_event(event);
  }

  void camera::on_simulate(const double) {}
  void camera::simulate(const double tick)
  {
    if (active.phase != help::phase::CREATED) throw exception("Camera must be created before simulation");
    active.timer.update(tick);
    on_simulate(tick);
  }

  void camera::on_render(const double) {}
  std::pair<glm::dmat4, glm::dmat4> camera::render(const double aspect, const double alpha)
  {
    if (active.phase != help::phase::CREATED) throw exception("Camera must be created before rendering");
    on_render(alpha);
    return {active.calculate_projection_matrix(previous, aspect, alpha), active.calculate_view_matrix(previous, alpha)};
  }

  void camera::on_destroy() {}
  void camera::destroy()
  {
    if (active.phase != help::phase::CREATED) throw exception("Camera must be created before destruction");
    active.phase = help::phase::PREPARED;
    on_destroy();
  }

  void camera::on_clean() {}
  void camera::clean()
  {
    if (active.phase != help::phase::PREPARED) throw exception("Camera must be prepared before cleaning");
    active.phase = help::phase::CLEANED;
    on_clean();
  }
}
