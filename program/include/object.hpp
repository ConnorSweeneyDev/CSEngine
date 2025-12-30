#pragma once

#include <cstddef>
#include <memory>
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
#include "hook.hpp"
#include "resource.hpp"
#include "state.hpp"

namespace cse
{
  class object
  {
    friend class scene;

  public:
    object(const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform_, const glm::u8vec4 &tint_,
           const std::pair<vertex, fragment> &shader_,
           const std::tuple<image, group, std::size_t, double, bool> &texture_);
    virtual ~object();
    object(const object &) = delete;
    object &operator=(const object &) = delete;
    object(object &&) = delete;
    object &operator=(object &&) = delete;

  private:
    void initialize(SDL_Window *instance, SDL_GPUDevice *gpu);
    void event(const SDL_Event &event);
    void input(const bool *keys);
    void simulate(const double poll_rate);
    void render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix, const double alpha,
                const float scale_factor);
    void cleanup(SDL_GPUDevice *gpu);

  public:
    std::weak_ptr<class scene> parent{};
    help::object_state state{};
    help::object_graphics graphics{};
    help::hooks hooks{};

  private:
    bool initialized{};
  };
}
