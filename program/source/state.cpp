#include "state.hpp"

#include <cmath>

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"

namespace cse::helper
{
  camera_state::camera_state(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_)
    : translation(translation_), forward(forward_), up(up_)
  {
  }

  glm::mat4 camera_state::calculate_view_matrix(const float global_scale_factor) const
  {
    return glm::lookAt(translation.interpolated * global_scale_factor,
                       (translation.interpolated * global_scale_factor) + forward.interpolated, up.interpolated);
  }

  object_state::object_state(const glm::vec3 &translation_, const glm::vec3 &rotation_, const glm::vec3 &scale_)
    : translation(translation_), rotation(rotation_), scale(scale_)
  {
  }

  glm::mat4 object_state::calculate_model_matrix(const unsigned int frame_width, const unsigned int frame_height,
                                                 const float global_scale_factor) const
  {
    glm::mat4 model_matrix{glm::mat4(1.0f)};
    model_matrix = glm::translate(
      model_matrix,
      {std::floor((translation.interpolated.x * global_scale_factor) / global_scale_factor) * global_scale_factor,
       std::floor((translation.interpolated.y * global_scale_factor) / global_scale_factor) * global_scale_factor,
       std::floor((translation.interpolated.z * global_scale_factor) / global_scale_factor) * global_scale_factor});
    model_matrix =
      glm::rotate(model_matrix, glm::radians(std::floor((rotation.interpolated.x * 90.0f) / 90.0f) * 90.0f),
                  glm::vec3(1.0f, 0.0f, 0.0f));
    model_matrix =
      glm::rotate(model_matrix, glm::radians(std::floor((rotation.interpolated.y * 90.0f) / 90.0f) * 90.0f),
                  glm::vec3(0.0f, 1.0f, 0.0f));
    model_matrix =
      glm::rotate(model_matrix, glm::radians(std::floor((rotation.interpolated.z * 90.0f) / 90.0f) * 90.0f),
                  glm::vec3(0.0f, 0.0f, 1.0f));
    model_matrix =
      glm::scale(model_matrix, {std::floor(scale.interpolated.x) * (static_cast<float>(frame_width) / 50.0f),
                                std::floor(scale.interpolated.y) * (static_cast<float>(frame_height) / 50.0f),
                                std::floor(scale.interpolated.z)});
    return model_matrix;
  }
}
