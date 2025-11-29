#pragma once

#include <exception>
#include <format>
#include <mutex>
#include <string>

namespace cse::utility
{
  inline static std::mutex print_mutex = {};

  void print(const std::string &message);

  template <typename... arguments>
  concept formattable_arguments = requires { typename std::format_string<arguments...>; };
  template <typename... message_arguments>
    requires formattable_arguments<message_arguments...>
  void print_format(std::format_string<message_arguments...> message, message_arguments &&...arguments);
}

namespace cse::utility
{
  class exception : public std::exception
  {
  public:
    template <typename... message_arguments> exception(const std::string &message_, message_arguments &&...arguments_);

    const char *what() const noexcept override;

  protected:
    std::string message = {};
  };

  class sdl_exception : public exception
  {
  public:
    template <typename... message_arguments>
    sdl_exception(const std::string &message_, message_arguments &&...arguments_);
  };
}

#include "utility.inl"
