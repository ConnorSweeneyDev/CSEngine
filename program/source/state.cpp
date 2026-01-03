#include "state.hpp"

#include <cmath>

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "glm/trigonometric.hpp"

#include "transform.hpp"

namespace cse::help
{
  game_state::game_state(const double poll_rate_) : poll_rate{poll_rate_} {}

  game_state::~game_state()
  {
    if (next_scene.has_value())
    {
      next_scene->second.reset();
      next_scene.reset();
    }
    scene.reset();
  }

  window_state::window_state(const glm::uvec2 &dimensions_, const bool fullscreen_, const bool vsync_)
    : width{dimensions_.x}, height{dimensions_.y}, fullscreen{fullscreen_}, vsync{vsync_}
  {
  }

  camera_state::camera_state(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_)
    : translation{translation_}, forward{forward_}, up{up_}
  {
  }

  glm::mat4 camera_state::calculate_view_matrix(const float scale_factor) const
  {
    return glm::lookAt(translation.interpolated * scale_factor,
                       (translation.interpolated * scale_factor) + forward.interpolated, up.interpolated);
  }

  object_state::object_state(const glm::ivec3 &translation_, const glm::ivec3 &rotation_, const glm::ivec3 &scale_)
    : translation{translation_}, rotation{rotation_}, scale{scale_}
  {
  }

  glm::mat4 object_state::calculate_model_matrix(const unsigned int frame_width, const unsigned int frame_height,
                                                 const float scale_factor) const
  {
    glm::mat4 model_matrix{glm::mat4(1.0f)};
    model_matrix = glm::translate(model_matrix, {std::floor(translation.interpolated.x) * scale_factor,
                                                 std::floor(translation.interpolated.y) * scale_factor,
                                                 std::floor(translation.interpolated.z) * scale_factor});
    model_matrix =
      glm::rotate(model_matrix, glm::radians(std::floor(rotation.interpolated.x) * 90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model_matrix =
      glm::rotate(model_matrix, glm::radians(std::floor(rotation.interpolated.y) * 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model_matrix =
      glm::rotate(model_matrix, glm::radians(std::floor(rotation.interpolated.z) * 90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model_matrix =
      glm::scale(model_matrix, {std::floor(scale.interpolated.x) * (static_cast<float>(frame_width) / 50.0f),
                                std::floor(scale.interpolated.y) * (static_cast<float>(frame_height) / 50.0f),
                                std::floor(scale.interpolated.z)});
    return model_matrix;
  }
}
