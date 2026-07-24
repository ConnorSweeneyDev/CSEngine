#pragma once

#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <mutex>
#include <string>
#include <utility>

#include "SDL3/SDL_error.h"
#include "SDL3/SDL_filesystem.h"

#include "core.hpp"
#include "print.hpp"

namespace cse
{
  namespace help
  {
    inline std::mutex log_mutex{};
    inline bool log_started{};
  }

  template <typename... message_arguments>
  void log(std::format_string<message_arguments...> message, message_arguments &&...arguments)
  {
    auto formatted_message{std::format(message, std::forward<message_arguments>(arguments)...)};
    print<CLOG>("{}.\n", formatted_message);
    const char *directory{SDL_GetPrefPath(help::meta.organization.c_str(), help::meta.application.c_str())};
    if (!directory)
    {
      print<CLOG>("Could not resolve the log directory; skipping log write\n");
      return;
    }
    const std::scoped_lock<std::mutex> lock(help::log_mutex);
    std::ofstream stream{std::filesystem::path(directory) / "log.txt",
                         help::log_started ? std::ios::app : std::ios::trunc};
    if (!stream) return;
    help::log_started = true;
    stream << formatted_message << ".\n";
  }

  template <typename... message_arguments>
  void sdl_log(std::format_string<message_arguments...> message, message_arguments &&...arguments)
  {
    auto formatted_message{std::format(message, std::forward<message_arguments>(arguments)...)};
    if (const std::string sdl_error{SDL_GetError()}; sdl_error.empty())
      log("{}: Unknown SDL error", formatted_message);
    else
      log("{}: {}", formatted_message, sdl_error);
  }
}
