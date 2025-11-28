#include "print.hpp"

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
