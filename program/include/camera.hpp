#pragma once

#include <functional>

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

namespace cse::base
{
  class camera
  {
  private:
    struct transform
    {
      struct property
      {
        property(const glm::vec3 &starting_value);

        glm::vec3 current;
        glm::vec3 previous;
        glm::vec3 interpolated;
        glm::vec3 velocity;
        glm::vec3 acceleration;
      };

      transform(const glm::vec3 &starting_translation, const glm::vec3 &starting_forward, const glm::vec3 &starting_up);

      property translation;
      property forward;
      property up;
    };
    struct graphics
    {
      glm::mat4 projection_matrix = glm::mat4(1.0f);
      glm::mat4 view_matrix = glm::mat4(1.0f);
    };

  public:
    camera(const glm::vec3 &starting_translation, const glm::vec3 &starting_forward, const glm::vec3 &starting_up,
           const float starting_fov, const float starting_near_clip, const float starting_far_clip);
    virtual ~camera();

    void input(const bool *key_state);
    void simulate(double simulation_alpha);
    void render(int width, int height);

  protected:
    std::function<void(const bool *key_state)> handle_input = nullptr;
    std::function<void(double simulation_alpha)> handle_simulate = nullptr;

  public:
    graphics graphics;

  protected:
    float fov;
    float near_clip;
    float far_clip;
    transform transform;
  };
}
