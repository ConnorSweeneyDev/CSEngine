#pragma once

#include "exception.hpp"

#include <format>
#include <string>

#include "SDL3/SDL_error.h"

namespace cse::utility
{
  template <typename... args> exception::exception(const std::string &format, args &&...arguments)
    : message(std::format("{}.", std::vformat(format, std::make_format_args(std::forward<const args>(arguments)...))))
  {
  }

  template <typename... args> sdl_exception::sdl_exception(const std::string &format, args &&...arguments)
    : exception(format, std::forward<args>(arguments)...)
  {
    message.pop_back();
    std::string sdl_error(SDL_GetError());
    if (sdl_error.empty())
      message = std::format("{}: Unknown SDL error.", message);
    else
      message = std::format("{}: {}.", message, sdl_error);
  }
}
