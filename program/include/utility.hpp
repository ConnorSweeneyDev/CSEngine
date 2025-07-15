#pragma once

#include <string>

namespace cse::utility
{
  enum Log_level
  {
    SUCCESS,
    FAILURE,
    SDL_FAILURE,
    TRACE,
  };
  int log(const std::string &message, Log_level result);
}
