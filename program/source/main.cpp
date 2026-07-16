#include "main.hpp"

#include <cstddef>
#include <exception>
#include <span>
#include <string_view>
#include <vector>

#include "SDL3/SDL_main.h"

#include "exception.hpp"
#include "system.hpp"

int main(int argc, char *argv[])
#undef main
{
  try
  {
    const std::span<char *> arguments(argv, static_cast<std::size_t>(argc));
    return cse::main(std::vector<std::string_view>(arguments.begin(), arguments.end()));
  }
  catch (const std::exception &error)
  {
    cse::exception::report(error);
    return cse::failure;
  }
}
