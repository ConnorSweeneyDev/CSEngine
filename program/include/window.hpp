#pragma once

#include <string>

#include "glm/ext/vector_uint2.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "hook.hpp"
#include "state.hpp"
#include "wrapper.hpp"

namespace cse
{
  class window
  {
    friend class game;

  protected:
    struct hook : public enumeration<hook>
    {
      using enumeration::enumeration;
      static constexpr enumeration_value<hook> PREPARE{};
      static constexpr enumeration_value<hook> CREATE{};
      static constexpr enumeration_value<hook> EVENT{};
      static constexpr enumeration_value<hook> INPUT{};
      static constexpr enumeration_value<hook> SIMULATE{};
      static constexpr enumeration_value<hook> PRE_RENDER{};
      static constexpr enumeration_value<hook> POST_RENDER{};
      static constexpr enumeration_value<hook> DESTROY{};
      static constexpr enumeration_value<hook> CLEAN{};
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
    help::hook hook{};
  };
}
