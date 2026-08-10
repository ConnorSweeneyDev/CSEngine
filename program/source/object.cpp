#include "object.hpp"

#include <cmath>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/trigonometric.hpp"

#include "core.hpp"
#include "exception.hpp"
#include "resource.hpp"
#include "temporal.hpp"
#include "transform.hpp"

namespace cse::help::object
{
  active::active(const temporal<glm::dvec3> &translation_, const temporal<double> &rotation_,
                 const temporal<glm::dvec2> &scale_, const object::collider &collider_, const object::texture &texture_,
                 const object::text &text_, const object::priority &priority_)
    : translation{translation_}, rotation{rotation_}, scale{scale_}, collider{collider_}, texture{texture_},
      text{text_}, priority{priority_} {};

  void active::synchronize(previous &last)
  {
    last.translation = translation;
    last.rotation = rotation;
    last.scale = scale;
    last.collider = collider;
    last.texture = texture;
    last.text = text;
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
    scale.instant = false;
    texture.playback.speed.instant = false;
    texture.color.tint.instant = false;
    texture.color.alpha.instant = false;
    texture.illumination.brightness.instant = false;
    texture.illumination.penetration.instant = false;
    texture.shadow.darkness.instant = false;
    texture.shadow.softness.instant = false;
    text.playback.speed.instant = false;
    text.align.horizontal.spacing.instant = false;
    text.align.vertical.spacing.instant = false;
    text.align.offset.instant = false;
    text.scale.instant = false;
    text.color.tint.instant = false;
    text.color.alpha.instant = false;
    text.illumination.brightness.instant = false;
    text.illumination.penetration.instant = false;
    text.shadow.darkness.instant = false;
    text.shadow.softness.instant = false;
  }

  glm::dmat4 active::calculate_model_matrix(const previous &last, const unsigned int frame_width,
                                            const unsigned int frame_height, const glm::dvec2 &pivot,
                                            const double alpha) const
  {
    auto interpolated_translation = translation.interpolated(last.translation, alpha);
    auto interpolated_rotation = rotation.interpolated(last.rotation, alpha);
    auto interpolated_scale = scale.interpolated(last.scale, alpha);
    const auto scale_x{std::floor(interpolated_scale.x + 0.5)};
    const auto scale_y{std::floor(interpolated_scale.y + 0.5)};
    const int steps{static_cast<int>(std::floor(interpolated_rotation + 0.5))};
    const auto offset{transform::anchor(steps, texture.flip, scale_x, scale_y, frame_width, frame_height, pivot)};
    auto model_matrix{glm::dmat4(1.0)};
    model_matrix = glm::translate(model_matrix, {std::floor(interpolated_translation.x + 0.5) + offset.x,
                                                 std::floor(interpolated_translation.y + 0.5) + offset.y,
                                                 std::floor(interpolated_translation.z + 0.5)});
    model_matrix = glm::rotate(model_matrix, 0.0, {1.0, 0.0, 0.0});
    model_matrix = glm::rotate(model_matrix, 0.0, {0.0, 1.0, 0.0});
    model_matrix =
      glm::rotate(model_matrix, glm::radians(std::floor(interpolated_rotation + 0.5) * -90.0), {0.0, 0.0, 1.0});
    model_matrix = glm::scale(model_matrix, {scale_x * static_cast<double>(frame_width) / 2.0,
                                             scale_y * static_cast<double>(frame_height) / 2.0, 1.0});
    return model_matrix;
  }

  glm::dmat4 active::calculate_text_matrix(const previous &last, const double width, const double height,
                                           const glm::dvec2 &offset, const double alpha) const
  {
    auto interpolated_translation = translation.interpolated(last.translation, alpha);
    auto interpolated_rotation = rotation.interpolated(last.rotation, alpha);
    const auto pixel_width{std::floor(width + 0.5)};
    const auto pixel_height{std::floor(height + 0.5)};
    auto text_matrix{glm::dmat4(1.0)};
    text_matrix = glm::translate(text_matrix, {std::floor(interpolated_translation.x + 0.5),
                                               std::floor(interpolated_translation.y + 0.5),
                                               std::floor(interpolated_translation.z + 0.5)});
    text_matrix =
      glm::rotate(text_matrix, glm::radians(std::floor(interpolated_rotation + 0.5) * -90.0), {0.0, 0.0, 1.0});
    text_matrix = glm::translate(
      text_matrix, {transform::snap_x(offset.x, pixel_width), transform::snap_y(offset.y, pixel_height), 0.0});
    text_matrix = glm::scale(text_matrix, {pixel_width / 2.0, pixel_height / 2.0, 1.0});
    return text_matrix;
  }

  void active::animate(const double tick)
  {
    const auto step{[tick](const cse::animation &animation, cse::playback &playback)
                    {
                      auto no_frames{animation.frames.empty()};
                      auto frame_count{animation.frames.size()};
                      if (no_frames)
                        playback.frame = 0;
                      else if (playback.frame >= frame_count)
                        playback.frame = frame_count - 1;
                      if (!playback.playing) return;
                      if (playback.speed.value > 0.0 && !no_frames)
                      {
                        playback.elapsed += tick * playback.speed.value;
                        while (true)
                        {
                          auto duration = animation.frames[playback.frame].duration;
                          if (duration > 0 && playback.elapsed < duration) break;
                          if (playback.frame < frame_count - 1)
                          {
                            if (duration > 0) playback.elapsed -= duration;
                            playback.frame++;
                          }
                          else if (playback.loop)
                          {
                            if (duration > 0)
                              playback.elapsed -= duration;
                            else
                              break;
                            playback.frame = 0;
                          }
                          else
                          {
                            if (duration > 0) playback.elapsed = duration;
                            break;
                          }
                        }
                      }
                      else if (playback.speed.value < 0.0 && !no_frames)
                      {
                        playback.elapsed += tick * playback.speed.value;
                        while (playback.elapsed < 0)
                          if (playback.frame > 0)
                          {
                            playback.frame--;
                            auto duration = animation.frames[playback.frame].duration;
                            if (duration > 0) playback.elapsed += duration;
                          }
                          else if (playback.loop)
                          {
                            if (animation.frames[0].duration <= 0) break;
                            playback.frame = frame_count - 1;
                            auto duration = animation.frames[playback.frame].duration;
                            if (duration > 0) playback.elapsed += duration;
                          }
                          else
                          {
                            playback.elapsed = 0;
                            break;
                          }
                      }
                    }};
    step(texture.source.animation, texture.playback);
    step(text.source.animation, text.playback);
  }
}

namespace cse
{
  object::object(const initial &initial_)
    : active{initial_.translation, initial_.rotation, initial_.scale,   initial_.collider,
             initial_.texture,     initial_.text,     initial_.priority} {};

  void object::on_prepare() {}
  void object::prepare()
  {
    if (active.phase != help::phase::CLEANED)
      throw exception("Object '{}' must be cleaned before preparation", name.string());
    active.phase = help::phase::PREPARED;
    on_prepare();
  }

  void object::on_create() {}
  void object::create()
  {
    if (active.phase != help::phase::PREPARED)
      throw exception("Object '{}' must be prepared before creation", name.string());
    active.phase = help::phase::CREATED;
    on_create();
  }

  void object::on_synchronize() {}
  void object::synchronize()
  {
    if (active.phase != help::phase::CREATED)
      throw exception("Object '{}' must be created before synchronization", name.string());
    active.synchronize(previous);
    on_synchronize();
  }

  void object::on_event(const SDL_Event &) {}
  void object::event(const SDL_Event &event)
  {
    if (active.phase != help::phase::CREATED)
      throw exception("Object '{}' must be created before processing events", name.string());
    on_event(event);
  }

  void object::on_simulate(const double) {}
  void object::simulate(const double tick)
  {
    if (active.phase != help::phase::CREATED)
      throw exception("Object '{}' must be created before simulation", name.string());
    active.timer.simulate(tick);
    active.mixer.simulate(tick);
    active.animate(tick);
    on_simulate(tick);
  }

  void object::on_collide(const double) {}
  void object::collide(const double tick)
  {
    if (active.phase != help::phase::CREATED)
      throw exception("Object '{}' must be created before simulation", name.string());
    on_collide(tick);
  }

  void object::on_destroy() {}
  void object::destroy()
  {
    if (active.phase != help::phase::CREATED)
      throw exception("Object '{}' must be created before destruction", name.string());
    active.phase = help::phase::PREPARED;
    on_destroy();
  }

  void object::on_clean() {}
  void object::clean()
  {
    if (active.phase != help::phase::PREPARED)
      throw exception("Object '{}' must be prepared before cleaning", name.string());
    active.phase = help::phase::CLEANED;
    on_clean();
  }
}
