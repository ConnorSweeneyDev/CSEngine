#pragma once

#include <functional>
#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/ext/vector_uint4_sized.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "resource.hpp"
#include "state.hpp"

namespace cse::core
{
  class object
  {
    friend class scene;

  public:
    object(const glm::ivec3 &translation_, const glm::ivec3 &rotation_, const glm::ivec3 &scale_,
           const resource::compiled_shader &vertex_shader_, const resource::compiled_shader &fragment_shader_,
           const resource::compiled_texture &texture_, const std::string &frame_group_, const glm::u8vec4 &tint_);
    virtual ~object();
    object(const object &) = delete;
    object &operator=(const object &) = delete;
    object(object &&) = delete;
    object &operator=(object &&) = delete;

  private:
    void initialize(SDL_Window *instance, SDL_GPUDevice *gpu);
    void cleanup(SDL_GPUDevice *gpu);
    void event(const SDL_Event &event);
    void input(const bool *keys);
    void simulate(double simulation_alpha);
    void render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix, const float global_scale_factor);

  protected:
    std::function<void(const SDL_Event &key)> handle_event = {};
    std::function<void(const bool *keys)> handle_input = {};
    std::function<void()> handle_simulate = {};

    helper::object_state state = {};
    helper::object_graphics graphics = {};
  };
}
