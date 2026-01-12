#include "state.hpp"

#include <cmath>
#include <tuple>

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
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

  game_state::~game_state()
  {
    if (next.scene.has_value()) next.scene->pointer.reset();
    next.scene.reset();
    if (next.window.has_value()) next.window->reset();
    next.window.reset();
    active.scene.pointer.reset();
    active.scenes.clear();
    active.window.reset();
    active.parent.reset();
    previous.scene.pointer.reset();
    previous.scene_names.clear();
    previous.window.reset();
  }

  void game_state::update_previous()
  {
    previous.phase = active.phase;
    previous.window = active.window;
    previous.scene_names.clear();
    previous.scene_names.reserve(active.scenes.size());
    for (const auto &[name, scene] : active.scenes) previous.scene_names.push_back(name);
    previous.scene = active.scene;
    previous.poll_rate = active.poll_rate;
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

  window_state::~window_state()
  {
    input = nullptr;
    active.vsync.change = nullptr;
    active.fullscreen.change = nullptr;
    active.display_index.change = nullptr;
    active.top.change = nullptr;
    active.left.change = nullptr;
    active.height.change = nullptr;
    active.width.change = nullptr;
    active.parent.reset();
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
  }

  scene_state::~scene_state()
  {
    next.camera.reset();
    active.objects.clear();
    active.camera.reset();
    active.parent.reset();
    previous.object_names.clear();
    previous.camera.reset();
  }

  void scene_state::update_previous()
  {
    previous.phase = active.phase;
    previous.camera = active.camera;
    previous.object_names.clear();
    previous.object_names.reserve(active.objects.size());
    for (const auto &[name, object] : active.objects) previous.object_names.push_back(name);
  }

  camera_state::camera_state(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform_)
    : previous{{}, std::get<0>(transform_), std::get<1>(transform_), std::get<2>(transform_)},
      active{{}, {}, std::get<0>(transform_), std::get<1>(transform_), std::get<2>(transform_)}
  {
  }

  camera_state::~camera_state() { active.parent.reset(); }

  glm::mat4 camera_state::calculate_view_matrix(const double alpha) const
  {
    auto translation = previous.translation.value +
                       (active.translation.value - previous.translation.value) * glm::vec3(static_cast<float>(alpha));
    auto forward =
      previous.forward.value + (active.forward.value - previous.forward.value) * glm::vec3(static_cast<float>(alpha));
    auto up = previous.up.value + (active.up.value - previous.up.value) * glm::vec3(static_cast<float>(alpha));
    return glm::lookAt(translation, translation + forward, up);
  }

  void camera_state::update_previous()
  {
    previous.phase = active.phase;
    previous.translation = active.translation;
    previous.forward = active.forward;
    previous.up = active.up;
  }

  object_state::object_state(const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform_)
    : previous{{},
               glm::vec3{std::get<0>(transform_)},
               glm::vec3{std::get<1>(transform_)},
               glm::vec3{std::get<2>(transform_)}},
      active{{},
             {},
             glm::vec3{std::get<0>(transform_)},
             glm::vec3{std::get<1>(transform_)},
             glm::vec3{std::get<2>(transform_)}}
  {
  }

  object_state::~object_state() { active.parent.reset(); }

  glm::mat4 object_state::calculate_model_matrix(const unsigned int frame_width, const unsigned int frame_height,
                                                 const double alpha) const
  {
    auto translation = previous.translation.value +
                       (active.translation.value - previous.translation.value) * glm::vec3(static_cast<float>(alpha));
    auto rotation = previous.rotation.value +
                    (active.rotation.value - previous.rotation.value) * glm::vec3(static_cast<float>(alpha));
    auto scale =
      previous.scale.value + (active.scale.value - previous.scale.value) * glm::vec3(static_cast<float>(alpha));
    glm::mat4 model_matrix{glm::mat4(1.0f)};
    model_matrix =
      glm::translate(model_matrix, {std::floor(translation.x + 0.5f) - (frame_width % 2 == 1 ? 0.5f : 0.0f),
                                    std::floor(translation.y + 0.5f) - (frame_height % 2 == 1 ? 0.5f : 0.0f),
                                    std::floor(translation.z + 0.5f)});
    model_matrix = glm::rotate(model_matrix, glm::radians(std::floor(rotation.x) * 90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model_matrix = glm::rotate(model_matrix, glm::radians(std::floor(rotation.y) * 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model_matrix = glm::rotate(model_matrix, glm::radians(std::floor(rotation.z) * 90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model_matrix =
      glm::scale(model_matrix, {std::floor(scale.x) * static_cast<float>(frame_width) / 2.0f,
                                std::floor(scale.y) * static_cast<float>(frame_height) / 2.0f, std::floor(scale.z)});
    return model_matrix;
  }

  void object_state::update_previous()
  {
    previous.phase = active.phase;
    previous.translation = active.translation;
    previous.rotation = active.rotation;
    previous.scale = active.scale;
  }
}
