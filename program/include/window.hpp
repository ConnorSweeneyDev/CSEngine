#pragma once

#include <functional>
#include <string>

#include "SDL3/SDL_events.h"

#include "declaration.hpp"
#include "graphics.hpp"

namespace cse::core
{
  class window
  {
    friend class game;

  public:
    window(const std::string &title_, const unsigned int starting_width_, const unsigned int starting_height_,
           const bool fullscreen_, const bool vsync_);
    virtual ~window();
    window(const window &) = delete;
    window &operator=(const window &) = delete;
    window(window &&) = delete;
    window &operator=(window &&) = delete;

  private:
    void initialize();
    void cleanup();
    void input();
    bool start_render();
    void end_render();

  protected:
    std::function<void(const SDL_KeyboardEvent &key)> handle_event = {};
    std::function<void(const bool *key_state)> handle_input = {};

    bool running = {};
    helper::window_graphics graphics = {};

  private:
    const bool *keys = {};
  };
}
