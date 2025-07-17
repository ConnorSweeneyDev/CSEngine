#include "exception.hpp"

namespace cse
{
  const char *Exception::what() const noexcept { return message.c_str(); }
}
