#include "state.hpp"

#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>
#include <system_error>

#include "nlohmann/json_fwd.hpp"

#include "core.hpp"
#include "exception.hpp"
#include "log.hpp"
#include "meta.hpp"

namespace cse::help
{
  namespace state
  {
    void log(std::string_view reason) { cse::log("Could not parse state field \"{}\"{}", marker_trail, reason); }
  }

  marker::marker(const std::string_view name_) : length{marker_trail.size()}
  {
    if (!marker_trail.empty()) marker_trail += '.';
    marker_trail += name_;
  }

  marker::~marker() { marker_trail.resize(length); }
}

namespace cse
{
  state::state(const initial &initial_)
    : storage{[&initial_]()
              {
                if (initial_.storage.empty()) throw exception("State storage path cannot be empty");
                std::filesystem::path path{initial_.storage};
                path.make_preferred();
                return path;
              }()} {};

  bool state::read()
  {
    if (!meta.output)
    {
      log("No access to user local directory, skipping read", storage.string());
      return false;
    }

    std::filesystem::path file{meta.output.value() / storage += ".json"};
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
    if (!json.is_object())
    {
      discard(std::format("type must be object, but is {}", json.type_name()));
      return false;
    }

    document = &json;
    writing = false;
    fallback = false;
    try
    {
      enroll();
    }
    catch (const std::exception &read_error)
    {
      document = nullptr;
      discard(read_error.what());
      return false;
    }
    document = nullptr;
    if (fallback)
    {
      discard("fields were malformed");
      return false;
    }
    return true;
  }

  bool state::write()
  {
    if (!meta.output)
    {
      log("No access to user local directory, skipping write", storage.string());
      return false;
    }
    std::filesystem::path file{meta.output.value() / storage += ".json"};
    std::error_code error{};
    std::filesystem::create_directories(file.parent_path(), error);
    if (error)
    {
      log("Could not create directory for state file '{}'; skipping write", file.string());
      return false;
    }

    nlohmann::json json{};
    document = &json;
    writing = true;
    enroll();
    document = nullptr;
    auto temporary{file};
    temporary += ".tmp";
    {
      std::ofstream stream{temporary, std::ios_base::binary};
      if (!stream)
      {
        log("Could not open state file '{}' for writing; skipping write", temporary.string());
        return false;
      }
      stream << json.dump(2);
      stream.close();
      if (!stream)
      {
        log("Could not write state file '{}'; skipping write", temporary.string());
        std::filesystem::remove(temporary);
        return false;
      }
    }
    std::filesystem::rename(temporary, file, error);
    if (error)
    {
      log("Could not rename temporary state file to '{}'; skipping write", file.string());
      std::filesystem::remove(temporary);
      return false;
    }
    return true;
  }
}
