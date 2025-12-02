#pragma once

#include "exception.hpp"

#include <format>
#include <string>

#include "SDL3/SDL_error.h"

namespace cse::utility
{
  template <typename... message_arguments>
  exception::exception(const std::string &message_, message_arguments &&...arguments_)
    : message(std::format(
        "{}.", std::vformat(message_, std::make_format_args(std::forward<const message_arguments>(arguments_)...))))
  {
  }

  template <typename... message_arguments>
  sdl_exception::sdl_exception(const std::string &message_, message_arguments &&...arguments_)
    : exception(message_, std::forward<const message_arguments>(arguments_)...)
  {
    message.pop_back();
    std::string sdl_error(SDL_GetError());
    if (sdl_error.empty())
      message = std::format("{}: Unknown SDL error.", message);
    else
      message = std::format("{}: {}.", message, sdl_error);
  }
}
