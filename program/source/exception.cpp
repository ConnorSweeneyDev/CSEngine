#include "exception.hpp"

namespace cse
{
  const char *Exception::what() const noexcept
  {
    if (message.empty()) return "Unknown exception.";
    return message.c_str();
  }
}
