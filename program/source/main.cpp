#include "main.hpp"

#include <exception>

#include "SDL3/SDL_filesystem.h"
#include "SDL3/SDL_main.h"
#include "csp/csp.hpp"

#include "exception.hpp"
#include "print.hpp"

int main(int argc, char *argv[])
#undef main
{
  try
  {
    const char *base{SDL_GetBasePath()};
    if (!base) throw cse::exception("Failed to resolve the application directory: {}", base);
    csp::mount(base);
    return cse::main(argc, argv);
  }
  catch (const std::exception &error)
  {
    cse::print<CERR>("{}.\n", error.what());
    return cse::failure;
  }
}
