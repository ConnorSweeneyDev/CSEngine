#include "utility.hpp"

#include <string>

#include "SDL3/SDL_error.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"

namespace cse::utility
{
  SDL_AppResult log(const std::string &message, Log_level level)
  {
    switch (level)
    {
      case INFO: SDL_Log("INFO: %s", message.c_str()); break;
      case SUCCESS: SDL_Log("SUCCESS: %s", message.c_str()); break;
      case FAILURE: SDL_Log("FAILURE: %s", message.c_str()); break;
      case SDL_FAILURE: SDL_Log("FAILURE: %s: %s", message.c_str(), SDL_GetError()); break;
      default: SDL_Log("UNKNOWN: %s", message.c_str()); break;
    }
    return level == INFO ? SDL_APP_CONTINUE : level == SUCCESS ? SDL_APP_SUCCESS : SDL_APP_FAILURE;
  }
}
