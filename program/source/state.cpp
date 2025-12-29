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
  window_state::window_state(const glm::uvec2 &dimensions_, const bool fullscreen_, const bool vsync_)
    : width(dimensions_.x), height(dimensions_.y), fullscreen(fullscreen_), vsync(vsync_)
  {
  }

  camera_state::camera_state(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform_)
    : translation(std::get<0>(transform_)), forward(std::get<1>(transform_)), up(std::get<2>(transform_))
  {
  }

  glm::mat4 camera_state::calculate_view_matrix(const float scale_factor) const
  {
    return glm::lookAt(translation.interpolated * scale_factor,
                       (translation.interpolated * scale_factor) + forward.interpolated, up.interpolated);
  }

  object_state::object_state(const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform_)
    : translation(std::get<0>(transform_)), rotation(std::get<1>(transform_)), scale(std::get<2>(transform_))
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
