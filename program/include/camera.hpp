#pragma once

#include <array>
#include <functional>

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "transform.hpp"

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
    void input(const bool *key_state);
    void simulate(const double simulation_alpha);
    std::array<glm::mat4, 2> render(const unsigned int width, const unsigned int height, const float scale_factor);

  protected:
    std::function<void(const bool *key_state)> handle_input = {};
    std::function<void()> handle_simulate = {};

    helper::camera_transform transform = {};
    helper::camera_graphics graphics = {};
  };
}
