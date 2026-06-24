#pragma once

#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/vector_double4.hpp"

#include "core.hpp"
#include "graphics.hpp"
#include "input.hpp"
#include "state.hpp"

namespace cse
{
  class window
  {
    friend class game;

  protected:
    struct initial_state
    {
      const SDL_DisplayID display{};
      const int left{};
      const int top{};
      const unsigned int width{};
      const unsigned int height{};
      const cse::mouse::initial mouse{};
    };
    struct initial_graphics
    {
      const std::string title{};
      const bool fullscreen{};
      const bool vsync{};
    };

  public:
    virtual ~window() = default;
    window(const window &) = delete;
    window &operator=(const window &) = delete;
    window(window &&) = delete;
    window &operator=(window &&) = delete;

  protected:
    window(const initial_state &state_, const initial_graphics &graphics_);
    virtual void on_prepare();
    virtual void on_create();
    virtual void on_previous();
    virtual void on_event(const SDL_Event &event);
    virtual void on_simulate(const double tick);
    virtual void pre_render(const double alpha);
    virtual void post_render(const double alpha);
    virtual void on_destroy();
    virtual void on_clean();

  private:
    void prepare();
    void create();
    void previous();
    void event();
    void simulate(const double tick);
    bool start_render(const glm::dvec4 &previous_clear, const glm::dvec4 &active_clear, const double previous_aspect,
                      const double active_aspect, const double alpha);
    void end_render(const double alpha);
    void destroy();
    void clean();

  public:
    cse::game *game{};
    help::window_state state{};
    help::window_graphics graphics{};

  protected:
    static constexpr SDL_DisplayID PRIMARY{1000000};
    static constexpr int CENTER{2000000};
  };
}
