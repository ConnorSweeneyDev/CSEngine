#pragma once

#include <format>
#include <mutex>

enum print_stream
{
  COUT,
  CERR,
  CLOG
};

namespace cse
{
  template <print_stream stream, typename... message_arguments>
  void print(std::format_string<message_arguments...> message, message_arguments &&...arguments);

  inline static std::mutex print_mutex{};
}

#include "print.inl"
