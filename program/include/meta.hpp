#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace cse
{
  struct meta
  {
    struct initial
    {
      const std::string organization{"CSEngine"};
      const std::string application{"Base"};
      const std::string version{"1.0.0"};
    };

    std::string organization{};
    std::string application{};
    std::string version{};
    std::optional<std::filesystem::path> output{};
  } inline meta{};
}
