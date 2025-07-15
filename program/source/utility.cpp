#include "utility.hpp"

#include <string>

#include "SDL3/SDL_error.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"

namespace cse::utility
{
  SDL_AppResult log(const std::string &message, SDL_AppResult result, bool display_sdl_error)
  {
    switch (result)
    {
      case SDL_APP_CONTINUE: SDL_Log("INFO: %s", message.c_str()); break;
      case SDL_APP_SUCCESS: SDL_Log("SUCCESS: %s", message.c_str()); break;
      case SDL_APP_FAILURE:
        if (display_sdl_error)
          SDL_Log("FAILURE: %s: %s", message.c_str(), SDL_GetError());
        else
          SDL_Log("FAILURE: %s", message.c_str());
        break;
      default: SDL_Log("UNKNOWN: %s", message.c_str()); break;
    }
    return result;
  }
}
