#pragma once

#include "declaration.hpp"

#include "SDL3/SDL_events.h"

namespace cse::helper
{
  struct window_state
  {
    friend class core::game;
    friend class core::window;

  public:
    bool running = {};

  private:
    SDL_Event event = {};
    const bool *keys = {};
  };
}
