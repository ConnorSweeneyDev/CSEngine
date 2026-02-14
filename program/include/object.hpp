#pragma once

#include <memory>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_int3.hpp"

#include "collisions.hpp"
#include "declaration.hpp"
#include "enumeration.hpp"
#include "graphics.hpp"
#include "hooks.hpp"
#include "name.hpp"
#include "resource.hpp"
#include "state.hpp"
#include "timers.hpp"

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
      static inline const value COLLIDE{};
      static inline const value RENDER{};
      static inline const value DESTROY{};
      static inline const value CLEAN{};
    };

  public:
    virtual ~object() = default;
    object(const object &) = delete;
    object &operator=(const object &) = delete;
    object(object &&) = delete;
    object &operator=(object &&) = delete;

  protected:
    object(const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform_, const std::pair<vertex, fragment> &shader_,
           const std::tuple<image, animation, playback, flip, color, transparency> &texture_,
           const std::tuple<int> &property_);

  private:
    void prepare();
    void create(SDL_Window *instance, SDL_GPUDevice *gpu);
    void previous();
    void event(const SDL_Event &event);
    void input(const bool *input);
    void simulate(const double poll_rate);
    void collide(const double poll_rate, const name self,
                 const std::unordered_map<name, std::shared_ptr<object>> &objects);
    void render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                const glm::dmat4 &projection_matrix, const glm::dmat4 &view_matrix, const double alpha);
    void destroy(SDL_GPUDevice *gpu);
    void clean();

  public:
    help::object_state state{};
    help::object_graphics graphics{};
    help::hooks hooks{};
    help::timers timers{};
    help::collisions collisions{};
  };
}
