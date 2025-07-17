#pragma once

#include "exception.hpp"

#include <format>
#include <string>

#include "SDL3/SDL_error.h"

namespace cse
{
  template <typename... Args> Exception::Exception(const std::string &format, Args &&...args)
    : message(std::format("{}.", std::vformat(format, std::make_format_args(std::forward<const Args>(args)...))))
  {
  }

  template <typename... Args> SDL_exception::SDL_exception(const std::string &format, Args &&...args)
    : Exception(std::format("{}: {}", std::vformat(format, std::make_format_args(std::forward<const Args>(args)...)),
                            SDL_GetError()))
  {
  }
}
