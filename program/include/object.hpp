#pragma once

#include <memory>
#include <tuple>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_int3.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "hook.hpp"
#include "previous.hpp"
#include "state.hpp"

namespace cse
{
  class object
  {
    friend class scene;

  public:
    object(const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform_,
           const struct help::object_graphics::shader &shader_, const struct help::object_graphics::texture &texture_,
           const struct help::object_graphics::property &property_);
    virtual ~object();
    object(const object &) = delete;
    object &operator=(const object &) = delete;
    object(object &&) = delete;
    object &operator=(object &&) = delete;

  private:
    void initialize(SDL_Window *instance, SDL_GPUDevice *gpu);
    void event(const SDL_Event &event);
    void input(const bool *keys);
    void simulate(const double active_poll_rate);
    void render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix, const float scale_factor);
    void cleanup(SDL_GPUDevice *gpu);

  public:
    std::weak_ptr<scene> parent{};
    help::object_state state{};
    help::object_graphics graphics{};
    help::object_previous previous{};
    help::hook hook{};

  private:
    bool initialized{};
  };
}
