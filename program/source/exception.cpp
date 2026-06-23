#include "exception.hpp"

#include <exception>
#include <format>
#include <string>

#include "SDL3/SDL_messagebox.h"

#include "print.hpp"

namespace cse
{
  const char *exception::what() const noexcept
  {
    if (message.empty()) return "Unknown exception.";
    return message.c_str();
  }

  void exception::report(const std::exception &error)
  {
    if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", std::format("{}.", error.what()).c_str(),
                                  nullptr))
      print<CERR>("{}.\n", error.what());
  }
}
