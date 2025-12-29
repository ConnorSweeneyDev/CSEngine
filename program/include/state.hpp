#pragma once

#include "declaration.hpp"

#include <tuple>

#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/ext/vector_uint2.hpp"

#include "property.hpp"
#include "transform.hpp"

namespace cse::help
{
  struct window_state
  {
    friend class cse::window;

  public:
    window_state() = default;
    window_state(const glm::uvec2 &dimensions_, const bool fullscreen_, const bool vsync_);

  public:
    bool running{};
    property<unsigned int> width{};
    property<unsigned int> height{};
    property<int> left{};
    property<int> top{};
    property<SDL_DisplayID> display_index{};
    property<bool> fullscreen{};
    property<bool> vsync{};
  };

  struct camera_state
  {
    friend class cse::camera;

  public:
    camera_state() = default;
    camera_state(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform_);

  private:
    glm::mat4 calculate_view_matrix(const float scale_factor) const;

  public:
    transform_value translation{};
    transform_value forward{};
    transform_value up{};
  };

  struct object_state
  {
    friend class cse::object;

  public:
    object_state() = default;
    object_state(const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform_);

  private:
    glm::mat4 calculate_model_matrix(const unsigned int frame_width, const unsigned int frame_height,
                                     const float scale_factor) const;

  public:
    transform_value translation{};
    transform_value rotation{};
    transform_value scale{};
  };
}
