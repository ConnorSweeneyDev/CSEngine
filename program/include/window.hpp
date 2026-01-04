#pragma once

#include <string>

#include "glm/ext/vector_uint2.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "hook.hpp"
#include "state.hpp"

namespace cse
{
  class window
  {
    friend class game;

  public:
    window(const std::string &title_, const glm::uvec2 &dimensions_, const bool fullscreen_, const bool vsync_);
    virtual ~window();
    window(const window &) = delete;
    window &operator=(const window &) = delete;
    window(window &&) = delete;
    window &operator=(window &&) = delete;

  private:
    void initialize();
    void event();
    void input();
    void simulate(const double active_poll_rate);
    bool start_render(const float aspect_ratio);
    void end_render();
    void cleanup();

    void update_previous();

  public:
    help::window_state state{};
    help::window_graphics graphics{};
    help::hook hook{};
  };
}
