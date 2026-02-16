#pragma once

#include <string>

#include "SDL3/SDL_events.h"
#include "glm/ext/vector_double4.hpp"
#include "glm/ext/vector_uint2.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "state.hpp"

namespace cse
{
  class window
  {
    friend class game;

  public:
    virtual ~window() = default;
    window(const window &) = delete;
    window &operator=(const window &) = delete;
    window(window &&) = delete;
    window &operator=(window &&) = delete;

  protected:
    window(const std::string &title_, const glm::uvec2 &dimensions_, const bool fullscreen_, const bool vsync_);
    virtual void on_prepare() {};
    virtual void on_create() {};
    virtual void on_previous() {};
    virtual void on_event(const SDL_Event &) {};
    virtual void on_input(const bool *) {};
    virtual void on_simulate(const double) {};
    virtual void pre_render(const double) {};
    virtual void post_render(const double) {};
    virtual void on_destroy() {};
    virtual void on_clean() {};

  private:
    void prepare();
    void create();
    void previous();
    void event();
    void input();
    void simulate(const double tick);
    bool start_render(const glm::dvec4 &previous_clear, const glm::dvec4 &active_clear, const double previous_aspect,
                      const double active_aspect, const double alpha);
    void end_render(const double alpha);
    void destroy();
    void clean();

  public:
    help::window_state state{};
    help::window_graphics graphics{};
  };
}
