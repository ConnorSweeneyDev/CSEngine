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
#include "wrapper.hpp"

namespace cse
{
  class object
  {
    friend class scene;

  protected:
    struct hook : public enumeration<hook>
    {
      using enumeration::enumeration;
      static constexpr enumeration_value<hook> PREPARE{};
      static constexpr enumeration_value<hook> CREATE{};
      static constexpr enumeration_value<hook> EVENT{};
      static constexpr enumeration_value<hook> INPUT{};
      static constexpr enumeration_value<hook> SIMULATE{};
      static constexpr enumeration_value<hook> RENDER{};
      static constexpr enumeration_value<hook> DESTROY{};
      static constexpr enumeration_value<hook> CLEAN{};
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
  };
}
