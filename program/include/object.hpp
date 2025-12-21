#pragma once

#include <string>
#include <tuple>
#include <utility>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/ext/vector_uint4_sized.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "hooks.hpp"
#include "resource.hpp"
#include "state.hpp"

namespace cse::core
{
  class object
  {
    friend class scene;

  public:
    object(const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform_, const glm::u8vec4 &tint_,
           const std::pair<resource::compiled_shader, resource::compiled_shader> &shader_,
           const std::pair<resource::compiled_texture, std::string> &texture_);
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
    helper::hooks hooks{};
    helper::object_state state{};
    helper::object_graphics graphics{};
  };
}
