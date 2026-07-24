#include "state.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <string>
#include <system_error>
#include <utility>

#include "SDL3/SDL_filesystem.h"
#include "nlohmann/json_fwd.hpp"

#include "core.hpp"
#include "exception.hpp"
#include "log.hpp"

namespace cse
{
  state::state(const initial &initial_)
    : storage{[&initial_]()
              {
                if (initial_.storage.empty()) throw exception("State storage path cannot be empty");
                std::filesystem::path path{initial_.storage};
                path.make_preferred();
                return path;
              }()}
  { building = this; }

  bool state::read()
  {
    std::filesystem::path file{};
    try
    {
      const char *directory{SDL_GetPrefPath(help::meta.organization.c_str(), help::meta.application.c_str())};
      if (!directory) throw sdl_exception("Failed to resolve the state directory");
      file = directory / storage;
    }
    catch (const std::exception &error)
    {
      log("Could not read state '{}': {}", storage.string(), error.what());
      return false;
    }
    file += ".json";
    std::error_code error{};
    if (!std::filesystem::exists(file, error))
    {
      if (error)
      {
        log("Could not check state file '{}': {}", file.string(), error.message());
        return false;
      }
      return true;
    }

    const auto discard{[&file](const std::string &reason)
                       {
                         auto backup{file};
                         backup += ".bak";
                         log("Could not read state file '{}' ({}); renaming it to '{}' and using defaults",
                             file.string(), reason, backup.string());
                         std::error_code backup_error{};
                         std::filesystem::remove(backup, backup_error);
                         std::filesystem::rename(file, backup, backup_error);
                         if (backup_error) log("Could not rename state file '{}'", file.string());
                       }};
    nlohmann::json json{};
    {
      std::ifstream stream{file, std::ios::binary};
      if (!stream)
      {
        discard("the file could not be opened");
        return false;
      }
      try
      {
        stream >> json;
      }
      catch (const nlohmann::json::exception &parse_error)
      {
        stream.close();
        discard(parse_error.what());
        return false;
      }
    }
    try
    {
      for (const auto &object : entries) object.reader(json);
    }
    catch (const nlohmann::json::exception &read_error)
    {
      discard(read_error.what());
      return false;
    }
    return true;
  }

  bool state::write() const
  {
    std::filesystem::path file{};
    try
    {
      const char *directory{SDL_GetPrefPath(help::meta.organization.c_str(), help::meta.application.c_str())};
      if (!directory) throw sdl_exception("Failed to resolve the state directory");
      file = directory / storage;
    }
    catch (const std::exception &error)
    {
      log("Could not write state '{}': {}", storage.string(), error.what());
      return false;
    }
    file += ".json";
    std::error_code error{};
    std::filesystem::create_directories(file.parent_path(), error);
    if (error)
    {
      log("Could not create directory for state file '{}'; skipping write", file.string());
      return false;
    }

    nlohmann::json json{};
    for (const auto &object : entries) object.writer(json);
    std::ofstream stream{file, std::ios_base::binary};
    if (!stream)
    {
      log("Could not open state file '{}' for writing; skipping write", file.string());
      return false;
    }
    stream << json.dump(2);
    stream.flush();
    if (!stream)
    {
      log("Could not write state file '{}'", file.string());
      return false;
    }
    return true;
  }

  void state::enlist(std::function<void(nlohmann::json &json)> writer,
                     std::function<void(const nlohmann::json &json)> reader)
  { entries.push_back({std::move(writer), std::move(reader)}); }
}
