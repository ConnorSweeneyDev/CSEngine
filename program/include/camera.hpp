#pragma once

#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "hooks.hpp"
#include "state.hpp"

namespace cse::core
{
  class camera
  {
    friend class scene;

  public:
    camera(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_, const float fov_);
    virtual ~camera();
    camera(const camera &) = delete;
    camera &operator=(const camera &) = delete;
    camera(camera &&) = delete;
    camera &operator=(camera &&) = delete;

  private:
    void event(const SDL_Event &event);
    void input(const bool *keys);
    void simulate(const double simulation_alpha);
    std::pair<glm::mat4, glm::mat4> render(const float target_aspect_ratio, const float global_scale_factor);

  protected:
    helper::hooks hooks = {};
    helper::camera_state state = {};
    helper::camera_graphics graphics = {};
  };
}
