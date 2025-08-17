#pragma once

#include "declaration.hpp"

namespace cse::helper
{
  struct window_state
  {
    friend class core::game;
    friend class core::window;

  public:
    bool running = true;

  private:
    const bool *keys = nullptr;
  };
}
