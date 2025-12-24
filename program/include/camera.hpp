#pragma once

#include <memory>
#include <tuple>
#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "hooks.hpp"
#include "state.hpp"

namespace cse
{
  class camera
  {
    friend class scene;

  public:
    camera(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform_, const float fov_);
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

  public:
    std::weak_ptr<class scene> scene{};
    help::camera_state state{};
    help::camera_graphics graphics{};
    help::hooks hooks{};
  };
}
