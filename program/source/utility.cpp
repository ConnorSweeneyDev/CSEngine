#include "utility.hpp"

#include <cstdlib>
#include <string>

#include "SDL3/SDL_error.h"
#include "SDL3/SDL_log.h"

namespace cse::utility
{
  int log(const std::string &message, Log_level level)
  {
    switch (level)
    {
      case SUCCESS: SDL_Log("SUCCESS | %s", message.c_str()); break;
      case FAILURE: SDL_Log("FAILURE | %s", message.c_str()); break;
      case SDL_FAILURE: SDL_Log("FAILURE | %s: %s", message.c_str(), SDL_GetError()); break;
      case TRACE: SDL_Log("TRACE   | %s", message.c_str()); break;
      default: SDL_Log("UNKNOWN | %s", message.c_str()); break;
    }
    return (level == SUCCESS || level == TRACE) ? EXIT_SUCCESS : EXIT_FAILURE;
  }
}
