#include "main.hpp"

#include <cstdlib>
#include <exception>

#include "SDL3/SDL_main.h"

#include "print.hpp"

int main(int argc, char *argv[])
{
#undef main
  try
  {
    return cse::main(argc, argv);
  }
  catch (const std::exception &error)
  {
    cse::print<CERR>("{}\n", error.what());
    return EXIT_FAILURE;
  }
}
