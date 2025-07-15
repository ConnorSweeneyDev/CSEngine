#pragma once

#include <string>

#include "SDL3/SDL_init.h"

namespace cse::utility
{
  SDL_AppResult log(const std::string &message, SDL_AppResult result, bool display_sdl_error = false);
}
