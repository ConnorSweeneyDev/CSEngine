#include "csp.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <span>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #define NOMINMAX
  #include <windows.h>

  #include <fileapi.h>
  #include <handleapi.h>
  #include <memoryapi.h>
  #include <winnt.h>
#else
  #include <fcntl.h>
  #include <sys/mman.h>
  #include <sys/stat.h>
  #include <unistd.h>
#endif

#include "SDL3/SDL_filesystem.h"

#include "exception.hpp"

namespace csp
{
  namespace
  {
    const manifest *registered{};
    mapping current{};

    std::uint32_t fingerprint(std::span<const unsigned char> bytes)
    {
      std::uint32_t crc{0xFFFFFFFFu};
      for (const auto byte : bytes)
      {
        crc ^= byte;
        for (int bit{}; bit < 8; ++bit) crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
      }
      return ~crc;
    }
  }

  manifest::manifest(const char *path_, std::uint64_t signature_, std::span<const patch> patches_)
    : path{path_}, signature{signature_}, patches{patches_}
  { registered = this; }

  mapping::~mapping()
  {
    if (!data) return;
#ifdef _WIN32
    UnmapViewOfFile(data);
#else
    munmap(const_cast<unsigned char *>(data), length);
#endif
  }

  void mapping::open(const std::filesystem::path &file)
  {
#ifdef _WIN32
    void *handle{
      CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (handle == INVALID_HANDLE_VALUE) throw cse::exception("Failed to open csp file '{}'", file.string());
    LARGE_INTEGER bytes{};
    if (!GetFileSizeEx(handle, &bytes))
    {
      CloseHandle(handle);
      throw cse::exception("Failed to size csp file '{}'", file.string());
    }
    void *map{CreateFileMappingW(handle, nullptr, PAGE_READONLY, 0, 0, nullptr)};
    if (!map)
    {
      CloseHandle(handle);
      throw cse::exception("Failed to map csp file '{}'", file.string());
    }
    void *view{MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0)};
    CloseHandle(map);
    CloseHandle(handle);
    if (!view) throw cse::exception("Failed to view csp file '{}'", file.string());
    data = static_cast<const unsigned char *>(view);
    length = static_cast<std::size_t>(bytes.QuadPart);
#else
    const int descriptor{::open(file.c_str(), O_RDONLY)};
    if (descriptor < 0) throw cse::exception("Failed to open csp file '{}'", file.string());
    struct stat status{};
    if (fstat(descriptor, &status) != 0)
    {
      ::close(descriptor);
      throw cse::exception("Failed to size csp file '{}'", file.string());
    }
    void *address{mmap(nullptr, static_cast<std::size_t>(status.st_size), PROT_READ, MAP_PRIVATE, descriptor, 0)};
    ::close(descriptor);
    if (address == MAP_FAILED) throw cse::exception("Failed to map csp file '{}'", file.string());
    data = static_cast<const unsigned char *>(address);
    length = static_cast<std::size_t>(status.st_size);
#endif
  }

  const unsigned char *mapping::base() const { return data; }

  std::size_t mapping::size() const { return length; }

  void mount()
  {
    if (!registered) return;

    const char *base_path{SDL_GetBasePath()};
    if (!base_path) throw cse::exception("Failed to resolve the application directory");
    const std::filesystem::path file{std::filesystem::path{base_path} / registered->path};
    current.open(file);

    const unsigned char *base{current.base()};
    if (current.size() < sizeof(header)) throw cse::exception("Csp file '{}' is truncated", file.string());
    const header &head{*reinterpret_cast<const header *>(base)};
    if (std::memcmp(head.magic, magic, sizeof(magic)) != 0)
      throw cse::exception("Csp file '{}' is not a csp file", file.string());
    if (head.version != version)
      throw cse::exception("Csp file '{}' has unsupported version {}", file.string(), head.version);
    if (head.signature != registered->signature)
      throw cse::exception("Csp file '{}' does not match this build", file.string());
    if (current.size() != sizeof(header) + head.size)
      throw cse::exception("Csp file '{}' has an unexpected size", file.string());
    const std::span<const unsigned char> content{base + sizeof(header), head.size};
    if (fingerprint(content) != head.fingerprint) throw cse::exception("Csp file '{}' is corrupted", file.string());

    for (const auto &entry : registered->patches)
      *entry.target = std::span<const unsigned char>{base + entry.offset, entry.size};
  }
}
