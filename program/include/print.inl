#pragma once

#include "print.hpp"

#include <format>

namespace cse::utility
{
  template <typename... message_arguments>
  void print_format(std::format_string<message_arguments...> message, message_arguments &&...args)
  {
    print(std::format(message, std::forward<message_arguments>(args)...));
  }
}
