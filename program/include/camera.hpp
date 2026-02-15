#pragma once

#include <tuple>
#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_double3.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "state.hpp"

namespace cse
{
  class camera
  {
    friend class scene;

  public:
    virtual ~camera() = default;
    camera(const camera &) = delete;
    camera &operator=(const camera &) = delete;
    camera(camera &&) = delete;
    camera &operator=(camera &&) = delete;

  protected:
    camera(const std::tuple<glm::dvec3, glm::dvec3, glm::dvec3> &transform_, const double fov_);
    virtual void on_prepare() {};
    virtual void on_create() {};
    virtual void on_previous() {};
    virtual void on_event(const SDL_Event &) {};
    virtual void on_input(const bool *) {};
    virtual void on_simulate(const double) {};
    virtual void on_render(const double) {};
    virtual void on_destroy() {};
    virtual void on_clean() {};

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
  };
}
