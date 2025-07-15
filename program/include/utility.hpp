#pragma once

#include <string>

#include "SDL3/SDL_init.h"

namespace cse::utility
{
  enum Log_level
  {
    INFO,
    SUCCESS,
    FAILURE,
    SDL_FAILURE
  };
  SDL_AppResult log(const std::string &message, Log_level result);
}
