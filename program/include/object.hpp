#pragma once

#include <array>
#include <functional>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

#include "resource.hpp"

namespace cse::base
{
  class object
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

      transform(const glm::vec3 &starting_translation, const glm::vec3 &starting_rotation,
                const glm::vec3 &starting_scale);

      property translation;
      property rotation;
      property scale;
    };

  protected:
    struct graphics
    {
      struct position_color_vertex
      {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        Uint8 r = 0, g = 0, b = 0, a = 0;
      };
      struct shader
      {
        resource::compiled_shader vertex = {};
        resource::compiled_shader fragment = {};
      };

      graphics(const resource::compiled_shader &vertex_shader, const resource::compiled_shader &fragment_shader);

      shader shader;
      SDL_GPUGraphicsPipeline *pipeline = nullptr;
      SDL_GPUBuffer *vertex_buffer = nullptr;
      SDL_GPUBuffer *index_buffer = nullptr;

      inline static const std::array<graphics::position_color_vertex, 4> default_quad_vertices = {
        graphics::position_color_vertex{0.5f, 0.5f, 0.0f, 0, 0, 255, 255},
        graphics::position_color_vertex{0.5f, -0.5f, 0.0f, 0, 255, 0, 255},
        graphics::position_color_vertex{-0.5f, 0.5f, 0.0f, 255, 255, 255, 255},
        graphics::position_color_vertex{-0.5f, -0.5f, 0.0f, 255, 0, 0, 255},
      };
      inline static const std::array<Uint16, 6> default_quad_indices = {3, 1, 0, 3, 0, 2};
    };

  public:
    object(const glm::vec3 &starting_translation, const glm::vec3 &starting_rotation, const glm::vec3 &starting_scale,
           const resource::compiled_shader &vertex_shader, const resource::compiled_shader &fragment_shader);
    virtual ~object();

    void initialize(SDL_Window *instance, SDL_GPUDevice *gpu);
    void cleanup(SDL_GPUDevice *gpu);
    void input(const bool *key_state);
    void simulate(double simulation_alpha);
    void render(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix);

  protected:
    std::function<void(const bool *key_state)> handle_input = nullptr;
    std::function<void(double simulation_alpha)> handle_simulate = nullptr;

  protected:
    transform transform;
    graphics graphics;
  };
}
