#pragma once

#include "utility.hpp"

#include <format>
#include <iostream>
#include <mutex>
#include <string>

#include "SDL3/SDL_error.h"

namespace cse::utility
{
  template <print_stream stream, typename... message_arguments>
  void print(std::format_string<message_arguments...> message, message_arguments &&...arguments)
  {
    std::lock_guard<std::mutex> lock(print_mutex);
    auto formatted_message = std::format(message, std::forward<message_arguments>(arguments)...);
    if constexpr (stream == COUT)
    {
      std::cout << formatted_message;
      std::cout.flush();
    }
    else if constexpr (stream == CERR)
    {
      std::cerr << formatted_message;
      std::cerr.flush();
    }
    else if constexpr (stream == CLOG)
    {
      std::clog << formatted_message;
      std::clog.flush();
    }
    else
      throw exception("Invalid print stream specification");
  }
}

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
