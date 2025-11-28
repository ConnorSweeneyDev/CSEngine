#pragma once

#include <format>
#include <mutex>
#include <string>

namespace cse::utility
{
  void print(const std::string &message);
  template <typename... message_arguments>
  void print_format(std::format_string<message_arguments...> message, message_arguments &&...args);

  inline static std::mutex print_mutex = {};
}

#include "print.inl"
