#pragma once

#include <tuple>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_int3.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "hooks.hpp"
#include "state.hpp"
#include "timers.hpp"
#include "wrapper.hpp"

namespace cse
{
  class object
  {
    friend class scene;

  protected:
    struct hook : public enumeration<hook>
    {
      static inline const value PREPARE{};
      static inline const value CREATE{};
      static inline const value EVENT{};
      static inline const value INPUT{};
      static inline const value SIMULATE{};
      static inline const value RENDER{};
      static inline const value DESTROY{};
      static inline const value CLEAN{};
    };

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
    void prepare();
    void create(SDL_Window *instance, SDL_GPUDevice *gpu);
    void previous();
    void event(const SDL_Event &event);
    void input(const bool *input);
    void simulate(const float poll_rate);
    void render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix, const double alpha);
    void destroy(SDL_GPUDevice *gpu);
    void clean();

  public:
    help::object_state state{};
    help::object_graphics graphics{};
    help::hooks hooks{};
    help::timers timers{};
  };
}
