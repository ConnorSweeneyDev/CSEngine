#include "state.hpp"

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
  {
    entries.push_back({std::move(writer), std::move(reader)});
  }

  void state::read()
  {
    auto file{directory() / location};
    file += ".json";
    std::error_code error{};
    if (!std::filesystem::exists(file, error) || error) return;

    std::ifstream stream{file, std::ios::binary};
    if (!stream) throw exception("Could not open state file '{}' for reading", file.string());
    nlohmann::json json{};
    try
    {
      stream >> json;
    }
    catch (const nlohmann::json::exception &parse_error)
    {
      throw exception("Could not parse state file '{}': {}", file.string(), parse_error.what());
    }
    for (const auto &object : entries) object.reader(json);
  }

  void state::write() const
  {
    auto file{directory() / location};
    file += ".json";
    std::error_code error{};
    std::filesystem::create_directories(file.parent_path(), error);
    if (error) throw exception("Could not create directory for state file '{}'", file.string());

    nlohmann::json json{};
    for (const auto &object : entries) object.writer(json);
    std::ofstream stream{file, std::ios::binary | std::ios::trunc};
    if (!stream) throw exception("Could not open state file '{}' for writing", file.string());
    stream << json.dump(2);
    if (!stream) throw exception("Could not write state file '{}'", file.string());
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
