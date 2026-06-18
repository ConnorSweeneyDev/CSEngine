#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>

namespace csp
{
  inline constexpr char magic[4]{'C', 'S', 'P', '0'};
  inline constexpr std::uint32_t version{1};

  struct header
  {
    char magic[4];
    std::uint32_t version;
    std::uint32_t flags;
    std::uint32_t fingerprint;
    std::uint64_t signature;
    std::uint64_t size;
  };
  struct patch
  {
    std::span<const unsigned char> *target;
    std::uint64_t offset;
    std::uint64_t size;
  };
  struct manifest
  {
    manifest(const char *path, std::uint64_t signature, std::span<const patch> patches);
    const char *path;
    std::uint64_t signature;
    std::span<const patch> patches;
  };
  static_assert(sizeof(header) == 32);

  class mapping
  {
  public:
    mapping() = default;
    ~mapping();
    mapping(const mapping &) = delete;
    mapping &operator=(const mapping &) = delete;

    void open(const std::filesystem::path &file);
    const unsigned char *base() const;
    std::size_t size() const;

  private:
    const unsigned char *data{};
    std::size_t length{};
  };

  void mount();
}
