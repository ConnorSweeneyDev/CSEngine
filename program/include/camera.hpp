#pragma once

#include <tuple>
#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "hooks.hpp"
#include "state.hpp"
#include "timers.hpp"
#include "wrapper.hpp"

namespace cse
{
  class camera
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
    camera(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform_, const double fov_);
    virtual ~camera();
    camera(const camera &) = delete;
    camera &operator=(const camera &) = delete;
    camera(camera &&) = delete;
    camera &operator=(camera &&) = delete;

  private:
    void prepare();
    void create();
    void previous();
    void event(const SDL_Event &event);
    void input(const bool *input);
    void simulate(const float poll_rate);
    std::pair<glm::mat4, glm::mat4> render(const double alpha, const float aspect_ratio);
    void destroy();
    void clean();

  public:
    help::camera_state state{};
    help::camera_graphics graphics{};
    help::hooks hooks{};
    help::timers timers{};
  };
}
