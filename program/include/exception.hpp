#pragma once

#include <exception>
#include <string>

namespace cse
{
  class exception : public std::exception
  {
  public:
    template <typename... message_arguments> exception(const std::string &message_, message_arguments &&...arguments_);

    const char *what() const noexcept override;
    static void report(const std::exception &error);

  protected:
    std::string message{};
  };

  class sdl_exception : public exception
  {
  public:
    template <typename... message_arguments>
    sdl_exception(const std::string &message_, message_arguments &&...arguments_);
  };
}

#include "exception.inl" // IWYU pragma: keep
