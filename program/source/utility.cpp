#include "utility.hpp"

#include <iostream>
#include <mutex>
#include <string>

namespace cse::utility
{
  void print(const std::string &message)
  {
    std::lock_guard<std::mutex> lock(print_mutex);
    std::cout << message;
    std::cout.flush();
  }
}

namespace cse::utility
{
  const char *exception::what() const noexcept
  {
    if (message.empty()) return "Unknown exception.";
    return message.c_str();
  }
}
