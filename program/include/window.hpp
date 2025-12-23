#pragma once

#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/vector_uint2.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "hooks.hpp"
#include "property.hpp"

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
    void simulate();
    bool start_render(const float target_aspect_ratio);
    void end_render();
    void cleanup();

  public:
    bool running{};
    const std::string title{};
    helper::property<unsigned int> width{};
    helper::property<unsigned int> height{};
    helper::property<int> left{};
    helper::property<int> top{};
    helper::property<SDL_DisplayID> display_index{};
    helper::property<bool> fullscreen{};
    helper::property<bool> vsync{};

  protected:
    helper::hooks hooks{};

  private:
    helper::window_graphics graphics{};
    SDL_Event current_event{};
    const bool *current_keys{};
  };
}
