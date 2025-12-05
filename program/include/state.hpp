#pragma once

#include "declaration.hpp"

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

#include "transform.hpp"

namespace cse::helper
{
  struct window_state
  {
    friend class core::game;
    friend class core::window;

  public:
    bool running = {};

  private:
    SDL_Event event = {};
    const bool *keys = {};
  };

  struct camera_state
  {
    friend class core::camera;

  public:
    camera_state() = default;
    camera_state(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_);

  private:
    glm::mat4 calculate_view_matrix(const float global_scale_factor) const;

  public:
    transform_value translation = {};
    transform_value forward = {};
    transform_value up = {};
  };

  struct object_state
  {
    friend class core::object;

  public:
    object_state() = default;
    object_state(const glm::vec3 &translation_, const glm::vec3 &rotation_, const glm::vec3 &scale_);

  private:
    glm::mat4 calculate_model_matrix(const unsigned int frame_width, const unsigned int frame_height,
                                     const float global_scale_factor) const;

  public:
    transform_value translation = {};
    transform_value rotation = {};
    transform_value scale = {};
  };
}
