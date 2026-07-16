#pragma once

#include <cstdint>
#include <format>
#include <iostream>
#include <mutex>
#include <ostream>

enum output_stream : std::uint8_t
{
  COUT,
  CERR,
  CLOG
};

namespace cse
{
  namespace help { inline std::mutex print_mutex{}; }

  template <output_stream stream, typename... message_arguments>
  void print(std::format_string<message_arguments...> message, message_arguments &&...arguments)
  {
    static_assert(stream == COUT || stream == CERR || stream == CLOG, "Invalid print stream specification");
    const std::scoped_lock<std::mutex> lock(help::print_mutex);
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
