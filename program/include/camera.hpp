#pragma once

#include <memory>
#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "hook.hpp"
#include "previous.hpp"
#include "state.hpp"

namespace cse
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
    void initialize();
    void event(const SDL_Event &event);
    void input(const bool *keys);
    void simulate(const double active_poll_rate);
    std::pair<glm::mat4, glm::mat4> render(const double alpha, const float aspect_ratio, const float scale_factor);
    void cleanup();

  public:
    std::weak_ptr<scene> parent{};
    help::camera_state state{};
    help::camera_graphics graphics{};
    help::camera_previous previous{};
    help::hook hook{};

  private:
    bool initialized{};
  };
}
