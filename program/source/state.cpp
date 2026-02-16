#include "state.hpp"

#include <cmath>
#include <tuple>

#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "glm/trigonometric.hpp"

namespace cse::help
{
  game_state::game_state(const double tick_)
    : previous{[&]()
               {
                 struct previous temporary{};
                 temporary.tick = tick_;
                 return temporary;
               }()},
      active{[&]()
             {
               struct active temporary{};
               temporary.tick = tick_;
               return temporary;
             }()}
  {
  }

  void game_state::update_previous()
  {
    previous.phase = active.phase;
    previous.window = active.window;
    previous.scenes.clear();
    previous.scenes.reserve(active.scenes.size());
    for (const auto &[name, scene] : active.scenes) previous.scenes.insert(name);
    previous.scene = active.scene;
    previous.tick = active.tick;
    previous.timer = active.timer;
  }

  window_state::window_state(const glm::uvec2 &dimensions_, const bool fullscreen_, const bool vsync_)
    : previous{[&]()
               {
                 struct previous temporary{};
                 temporary.width = dimensions_.x;
                 temporary.height = dimensions_.y;
                 temporary.fullscreen = fullscreen_;
                 temporary.vsync = vsync_;
                 return temporary;
               }()},
      active{[&]()
             {
               struct active temporary{};
               temporary.width = dimensions_.x;
               temporary.height = dimensions_.y;
               temporary.fullscreen = fullscreen_;
               temporary.vsync = vsync_;
               return temporary;
             }()}
  {
  }

  void window_state::update_previous()
  {
    previous.phase = active.phase;
    previous.running = active.running;
    previous.width = active.width;
    previous.height = active.height;
    previous.left = active.left;
    previous.top = active.top;
    previous.display_index = active.display_index;
    previous.fullscreen = active.fullscreen;
    previous.vsync = active.vsync;
    previous.timer = active.timer;
  }

  void scene_state::update_previous()
  {
    previous.phase = active.phase;
    previous.camera = active.camera;
    previous.objects.clear();
    previous.objects.reserve(active.objects.size());
    for (const auto &[name, object] : active.objects) previous.objects.insert(name);
    previous.timer = active.timer;
  }

  camera_state::camera_state(const std::tuple<glm::dvec3, glm::dvec3, glm::dvec3> &transform_)
    : previous{{}, std::get<0>(transform_), std::get<1>(transform_), std::get<2>(transform_)},
      active{{}, {}, std::get<0>(transform_), std::get<1>(transform_), std::get<2>(transform_)}
  {
  }

  void camera_state::update_previous()
  {
    previous.phase = active.phase;
    previous.translation = active.translation;
    previous.forward = active.forward;
    previous.up = active.up;
    previous.timer = active.timer;
  }

  glm::dmat4 camera_state::calculate_view_matrix(const double alpha) const
  {
    auto translation = previous.translation.value + (active.translation.value - previous.translation.value) * alpha;
    auto forward = previous.forward.value + (active.forward.value - previous.forward.value) * alpha;
    auto up = previous.up.value + (active.up.value - previous.up.value) * alpha;
    return glm::lookAt(translation, translation + forward, up);
  }

  object_state::object_state(const std::tuple<glm::dvec3, double, glm::dvec2> &transform_, const bool collidable_)
    : previous{{}, std::get<0>(transform_), std::get<1>(transform_), std::get<2>(transform_), collidable_},
      active{{}, {}, std::get<0>(transform_), std::get<1>(transform_), std::get<2>(transform_), collidable_}
  {
  }

  void object_state::update_previous()
  {
    previous.phase = active.phase;
    previous.translation = active.translation;
    previous.rotation = active.rotation;
    previous.scale = active.scale;
    previous.collidable = active.collidable;
    previous.timer = active.timer;
    previous.collision = active.collision;
  }

  glm::dmat4 object_state::calculate_model_matrix(const unsigned int frame_width, const unsigned int frame_height,
                                                  const double alpha) const
  {
    auto translation = previous.translation.value + (active.translation.value - previous.translation.value) * alpha;
    auto rotation = previous.rotation.value + (active.rotation.value - previous.rotation.value) * alpha;
    auto scale = previous.scale.value + (active.scale.value - previous.scale.value) * alpha;
    auto model_matrix{glm::dmat4(1.0)};
    model_matrix = glm::translate(model_matrix, {std::floor(translation.x + 0.5) - (frame_width % 2 == 1 ? 0.5 : 0.0),
                                                 std::floor(translation.y + 0.5) - (frame_height % 2 == 1 ? 0.5 : 0.0),
                                                 std::floor(translation.z + 0.5)});
    model_matrix = glm::rotate(model_matrix, 0.0, {1.0, 0.0, 0.0});
    model_matrix = glm::rotate(model_matrix, 0.0, {0.0, 1.0, 0.0});
    model_matrix = glm::rotate(model_matrix, glm::radians(std::floor(rotation + 0.5) * 90.0), {0.0, 0.0, 1.0});
    model_matrix = glm::scale(model_matrix, {std::floor(scale.x + 0.5) * static_cast<double>(frame_width) / 2.0,
                                             std::floor(scale.y + 0.5) * static_cast<double>(frame_height) / 2.0, 1.0});
    return model_matrix;
  }
}
