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
    private:
      struct property
      {
      public:
        property(const glm::vec3 &value_);

        glm::vec3 get_previous() const;
        glm::vec3 get_interpolated() const;
        void update_previous();
        void update_interpolated(float simulation_alpha);

      public:
        glm::vec3 value = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 acceleration = glm::vec3(0.0f, 0.0f, 0.0f);

      private:
        glm::vec3 previous = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 interpolated = glm::vec3(0.0f, 0.0f, 0.0f);
      };

    public:
      transform(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_);

    public:
      property translation = glm::vec3(0.0f, 0.0f, 0.0f);
      property forward = glm::vec3(0.0f, 0.0f, -1.0f);
      property up = glm::vec3(0.0f, 1.0f, 0.0f);
    };
    struct graphics
    {
      glm::mat4 projection_matrix = glm::mat4(1.0f);
      glm::mat4 view_matrix = glm::mat4(1.0f);
    };

  public:
    camera(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_, const float fov_,
           const float near_clip_, const float far_clip_);
    virtual ~camera();
    camera(const camera &) = delete;
    camera &operator=(const camera &) = delete;
    camera(camera &&) = delete;
    camera &operator=(camera &&) = delete;

    void input(const bool *key_state);
    void simulate(double simulation_alpha);
    void render(int width, int height);

    graphics get_graphics() const;

  protected:
    std::function<void(const bool *key_state)> handle_input = nullptr;
    std::function<void()> handle_simulate = nullptr;

    float fov = 0.0f;
    float near_clip = 0.0f;
    float far_clip = 0.0f;
    transform transform = {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f)};
    graphics graphics = {};
  };
}
