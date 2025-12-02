#include "exception.hpp"

#include <string>

namespace cse::utility
{
  const char *exception::what() const noexcept
  {
    if (message.empty()) return "Unknown exception.";
    return message.c_str();
  }
}
