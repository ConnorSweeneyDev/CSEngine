#pragma once

#include <array>
#include <functional>

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

#include "declaration.hpp"
#include "transform.hpp"

namespace cse::core
{
  class camera
  {
    friend class scene;

  private:
    struct graphics
    {
      friend class camera;

    public:
      graphics() = default;
      graphics(const float fov_);

    private:
      glm::mat4 calculate_projection_matrix(const unsigned int width, const unsigned int height);
      glm::mat4 calculate_view_matrix(const glm::vec3 &translation, const glm::vec3 &forward, const glm::vec3 &up,
                                      const float scale_factor);

    public:
      float fov = 0.0f;

    private:
      float near_clip = 0.0f;
      float far_clip = 0.0f;
      glm::mat4 projection_matrix = glm::mat4(1.0f);
      glm::mat4 view_matrix = glm::mat4(1.0f);
    };

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
    std::function<void(const bool *key_state)> handle_input = nullptr;
    std::function<void()> handle_simulate = nullptr;

    helper::camera_transform transform = {};
    graphics graphics = {};
  };
}
