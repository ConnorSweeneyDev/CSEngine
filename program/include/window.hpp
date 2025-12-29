#pragma once

#include <memory>
#include <string>

#include "SDL3/SDL_events.h"
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
    friend class scene;

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
    void simulate();
    bool start_render(const float target_aspect_ratio);
    void end_render();
    void cleanup();

  public:
    std::weak_ptr<class game> parent{};
    help::window_state state{};
    help::window_graphics graphics{};
    help::hooks hooks{};

  private:
    SDL_Event current_event{};
    const bool *current_keys{};
  };
}
