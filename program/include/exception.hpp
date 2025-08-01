#pragma once

#include <exception>
#include <string>

namespace cse::utility
{
  class exception : public std::exception
  {
  public:
    template <typename... args> exception(const std::string &format, args &&...arguments);

    const char *what() const noexcept override;

  protected:
    std::string message = "";
  };

  class sdl_exception : public exception
  {
  public:
    template <typename... args> sdl_exception(const std::string &format, args &&...arguments);
  };
}

#include "exception.inl"
