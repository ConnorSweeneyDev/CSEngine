#include "state.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <string>
#include <system_error>
#include <utility>

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>

  #include <combaseapi.h>
  #include <knownfolders.h>
  #include <shlobj.h>
  #include <shlobj_core.h>
  #include <winerror.h>
  #include <winnt.h>

  #ifdef near
    #undef near
  #endif
  #ifdef far
    #undef far
  #endif
#elif defined(__linux__)
  #include <cstdlib>
  #include <pwd.h>
  #include <unistd.h>
#endif

#include "nlohmann/json_fwd.hpp"

#include "exception.hpp"
#include "log.hpp"

namespace cse
{
  state::state(std::filesystem::path location_)
  {
    location_.make_preferred();
    location = std::move(location_);
    building = this;
  }

  void state::enlist(std::function<void(nlohmann::json &json)> writer,
                     std::function<void(const nlohmann::json &json)> reader)
  { entries.push_back({std::move(writer), std::move(reader)}); }

  void state::read()
  {
    std::filesystem::path file{};
    try
    {
      file = directory() / location;
    }
    catch (const std::exception &error)
    {
      log("Could not read state '{}': {}", location.string(), error.what());
      return;
    }
    file += ".json";
    std::error_code error{};
    if (!std::filesystem::exists(file, error) || error) return;

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
        return;
      }
      try
      {
        stream >> json;
      }
      catch (const nlohmann::json::exception &parse_error)
      {
        stream.close();
        discard(parse_error.what());
        return;
      }
    }
    try
    {
      for (const auto &object : entries) object.reader(json);
    }
    catch (const nlohmann::json::exception &read_error)
    {
      discard(read_error.what());
    }
  }

  void state::write() const
  {
    std::filesystem::path file{};
    try
    {
      file = directory() / location;
    }
    catch (const std::exception &error)
    {
      log("Could not write state '{}': {}", location.string(), error.what());
      return;
    }
    file += ".json";
    std::error_code error{};
    std::filesystem::create_directories(file.parent_path(), error);
    if (error)
    {
      log("Could not create directory for state file '{}'; skipping write", file.string());
      return;
    }

    nlohmann::json json{};
    for (const auto &object : entries) object.writer(json);
    std::ofstream stream{file, std::ios::binary | std::ios::trunc};
    if (!stream)
    {
      log("Could not open state file '{}' for writing; skipping write", file.string());
      return;
    }
    stream << json.dump(2);
    if (!stream) log("Could not write state file '{}'", file.string());
  }

  std::filesystem::path state::directory() const
  {
#if defined(_WIN32)
    PWSTR raw{};
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &raw)))
    {
      CoTaskMemFree(raw);
      throw exception("Could not resolve local application data directory");
    }
    std::filesystem::path directory{raw};
    CoTaskMemFree(raw);
    return directory;
#elif defined(__linux__)
    if (const char *data{std::getenv("XDG_DATA_HOME")}; data && *data) return std::filesystem::path{data};
    if (const char *home{std::getenv("HOME")}; home && *home) return std::filesystem::path{home} / ".local" / "share";
    if (const passwd *object{getpwuid(getuid())}; object && object->pw_dir)
      return std::filesystem::path{object->pw_dir} / ".local" / "share";
    throw exception("Could not resolve user data directory");
#endif
  }
}
