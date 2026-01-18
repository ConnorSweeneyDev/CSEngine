#pragma once

#include <string>

#include "glm/ext/vector_uint2.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "hooks.hpp"
#include "state.hpp"
#include "timers.hpp"
#include "wrapper.hpp"

namespace cse
{
  class window
  {
    friend class game;

  protected:
    struct hook : public enumeration<hook>
    {
      static inline const value PREPARE{};
      static inline const value CREATE{};
      static inline const value EVENT{};
      static inline const value INPUT{};
      static inline const value SIMULATE{};
      static inline const value PRE_RENDER{};
      static inline const value POST_RENDER{};
      static inline const value DESTROY{};
      static inline const value CLEAN{};
    };

  public:
    window(const std::string &title_, const glm::uvec2 &dimensions_, const bool fullscreen_, const bool vsync_);
    virtual ~window();
    window(const window &) = delete;
    window &operator=(const window &) = delete;
    window(window &&) = delete;
    window &operator=(window &&) = delete;

  private:
    void prepare();
    void create();
    void previous();
    void event();
    void input();
    void simulate(const float poll_rate);
    bool pre_render(const double alpha, const float aspect_ratio);
    void post_render(const double alpha);
    void destroy();
    void clean();

  public:
    help::window_state state{};
    help::window_graphics graphics{};
    help::hooks hooks{};
    help::timers timers{};
  };
}
