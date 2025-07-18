#pragma once

#include <exception>
#include <string>

namespace cse
{
  class Exception : public std::exception
  {
  public:
    template <typename... Args> Exception(const std::string &format, Args &&...args);
    const char *what() const noexcept override;

  protected:
    std::string message = "";
  };

  class SDL_exception : public Exception
  {
  public:
    template <typename... Args> SDL_exception(const std::string &format, Args &&...args);
  };
}

#include "exception.inl"
