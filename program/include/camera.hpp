#pragma once

#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_double3.hpp"

#include "core.hpp"
#include "graphics.hpp"
#include "state.hpp"

namespace cse
{
  class camera
  {
    friend class scene;

  protected:
    struct initial_state
    {
      const glm::dvec3 translation{};
      const glm::dvec3 forward{};
      const glm::dvec3 up{};
    };
    struct initial_graphics
    {
      const double fov{};
      const help::camera_graphics::clip clip{};
    };

  public:
    virtual ~camera() = default;
    camera(const camera &) = delete;
    camera &operator=(const camera &) = delete;
    camera(camera &&) = delete;
    camera &operator=(camera &&) = delete;

  protected:
    camera(const initial_state &state_, const initial_graphics &graphics_);
    virtual void on_prepare();
    virtual void on_create();
    virtual void on_previous();
    virtual void on_event(const SDL_Event &event);
    virtual void on_simulate(const double tick);
    virtual void on_render(const double alpha);
    virtual void on_destroy();
    virtual void on_clean();

  private:
    void prepare();
    void create();
    void previous();
    void event(const SDL_Event &event);
    void simulate(const double tick);
    std::pair<glm::dmat4, glm::dmat4> render(const double previous_aspect, const double active_aspect,
                                             const double alpha);
    void destroy();
    void clean();

  public:
    cse::scene *scene{};
    help::camera_state state{};
    help::camera_graphics graphics{};
  };
}
