#pragma once

#include <array>

#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_scancode.h"
#include "glm/ext/vector_double2.hpp"

namespace cse
{
  using keyboard = std::array<bool, SDL_SCANCODE_COUNT>;

  struct mouse
  {
    glm::dvec2 position{};
    std::array<bool, SDL_BUTTON_X2 + 1> buttons{};
    glm::dvec2 wheel{};
  };
}
