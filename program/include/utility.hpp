#pragma once

#include <exception>
#include <format>
#include <mutex>
#include <string>

namespace cse::utility
{
  enum print_stream
  {
    COUT,
    CERR,
    CLOG
  };

  template <print_stream stream, typename... message_arguments>
  void print(std::format_string<message_arguments...> message, message_arguments &&...arguments);

  inline static std::mutex print_mutex = {};
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
