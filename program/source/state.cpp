#include "state.hpp"

#include <cmath>
#include <tuple>

#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "glm/trigonometric.hpp"

namespace cse::help
{
  game_state::game_state(const double poll_rate_)
    : previous{[&]()
               {
                 struct previous temp{};
                 temp.poll_rate = poll_rate_;
                 return temp;
               }()},
      active{[&]()
             {
               struct active temp{};
               temp.poll_rate = poll_rate_;
               return temp;
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
    previous.poll_rate = active.poll_rate;
    previous.timer = active.timer;
  }

  window_state::window_state(const glm::uvec2 &dimensions_, const bool fullscreen_, const bool vsync_)
    : previous{[&]()
               {
                 struct previous temp{};
                 temp.width = dimensions_.x;
                 temp.height = dimensions_.y;
                 temp.fullscreen = fullscreen_;
                 temp.vsync = vsync_;
                 return temp;
               }()},
      active{[&]()
             {
               struct active temp{};
               temp.width = dimensions_.x;
               temp.height = dimensions_.y;
               temp.fullscreen = fullscreen_;
               temp.vsync = vsync_;
               return temp;
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

  object_state::object_state(const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform_, const bool collidable_)
    : previous{{},
               glm::dvec3{std::get<0>(transform_)},
               glm::dvec3{std::get<1>(transform_)},
               glm::dvec3{std::get<2>(transform_)},
               collidable_},
      active{{},
             {},
             glm::dvec3{std::get<0>(transform_)},
             glm::dvec3{std::get<1>(transform_)},
             glm::dvec3{std::get<2>(transform_)},
             collidable_}
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
    glm::dmat4 model_matrix{glm::dmat4(1.0)};
    model_matrix = glm::translate(model_matrix, {std::floor(translation.x + 0.5) - (frame_width % 2 == 1 ? 0.5 : 0.0),
                                                 std::floor(translation.y + 0.5) - (frame_height % 2 == 1 ? 0.5 : 0.0),
                                                 std::floor(translation.z + 0.5)});
    model_matrix = glm::rotate(model_matrix, glm::radians(std::floor(rotation.x) * 90.0), glm::dvec3{1.0, 0.0, 0.0});
    model_matrix = glm::rotate(model_matrix, glm::radians(std::floor(rotation.y) * 90.0), glm::dvec3{0.0, 1.0, 0.0});
    model_matrix = glm::rotate(model_matrix, glm::radians(std::floor(rotation.z) * 90.0), glm::dvec3{0.0, 0.0, 1.0});
    model_matrix =
      glm::scale(model_matrix, {std::floor(scale.x) * static_cast<double>(frame_width) / 2.0,
                                std::floor(scale.y) * static_cast<double>(frame_height) / 2.0, std::floor(scale.z)});
    return model_matrix;
  }
}
