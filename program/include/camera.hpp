#pragma once

#include <tuple>
#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_double3.hpp"

#include "declaration.hpp"
#include "enumeration.hpp"
#include "graphics.hpp"
#include "hooks.hpp"
#include "state.hpp"
#include "timers.hpp"

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
    virtual ~camera() = default;
    camera(const camera &) = delete;
    camera &operator=(const camera &) = delete;
    camera(camera &&) = delete;
    camera &operator=(camera &&) = delete;

  protected:
    camera(const std::tuple<glm::dvec3, glm::dvec3, glm::dvec3> &transform_, const double fov_);

  private:
    void prepare();
    void create();
    void previous();
    void event(const SDL_Event &event);
    void input(const bool *input);
    void simulate(const double poll_rate);
    std::pair<glm::dmat4, glm::dmat4> render(const double alpha, const double previous_aspect_ratio,
                                             const double active_aspect_ratio);
    void destroy();
    void clean();

  public:
    help::camera_state state{};
    help::camera_graphics graphics{};
    help::hooks hooks{};
    help::timers timers{};
  };
}
