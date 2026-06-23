#include "main.hpp"

#include <exception>

#include "SDL3/SDL_main.h"

#include "exception.hpp"
#include "system.hpp"

int main(int argc, char *argv[])
#undef main
{
  try
  {
    return cse::main(argc, argv);
  }
  catch (const std::exception &error)
  {
    cse::exception::report(error);
    return cse::failure;
  }
}
