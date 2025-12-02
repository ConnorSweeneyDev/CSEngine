#pragma once

#include "print.hpp"

#include <format>
#include <iostream>
#include <mutex>

namespace cse::utility
{
  template <print_stream stream, typename... message_arguments>
  void print(std::format_string<message_arguments...> message, message_arguments &&...arguments)
  {
    std::lock_guard<std::mutex> lock(print_mutex);
    auto formatted_message = std::format(message, std::forward<message_arguments>(arguments)...);
    std::ostream &choice = []() -> std::ostream &
    {
      if constexpr (stream == COUT)
        return std::cout;
      else if constexpr (stream == CERR)
        return std::cerr;
      else if constexpr (stream == CLOG)
        return std::clog;
      else
        throw exception("Invalid print stream specification");
    }();
    choice << formatted_message;
    choice.flush();
  }
}
