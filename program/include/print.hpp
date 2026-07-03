#pragma once

#include <format>
#include <iostream>
#include <mutex>
#include <ostream>

enum output_stream
{
  COUT,
  CERR,
  CLOG
};

namespace cse
{
  inline std::mutex print_mutex{};

  template <output_stream stream, typename... message_arguments>
  void print(std::format_string<message_arguments...> message, message_arguments &&...arguments)
  {
    static_assert(stream == COUT || stream == CERR || stream == CLOG, "Invalid print stream specification");
    std::lock_guard<std::mutex> lock(print_mutex);
    auto formatted_message{std::format(message, std::forward<message_arguments>(arguments)...)};
    auto &choice{[]() -> std::ostream &
                 {
                   if constexpr (stream == COUT)
                     return std::cout;
                   else if constexpr (stream == CERR)
                     return std::cerr;
                   else
                     return std::clog;
                 }()};
    choice << formatted_message;
    choice.flush();
  }
}
