#include "resource.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <span>
#include <utility>
#include <vector>
#if defined(_DEBUG)
  #include <string>
#endif

#include "SDL3/SDL_filesystem.h"
#include "csp/csp.hpp"

#include "collision.hpp"
#include "exception.hpp"
#include "numeric.hpp"
#include "print.hpp"
#include "system.hpp"

namespace cse::resource
{
  namespace
  {
    std::vector<animation::frame> &frame_storage()
    {
      static std::vector<animation::frame> instance{};
      return instance;
    }
    std::vector<std::pair<hitbox, rectangle>> &hitbox_storage()
    {
      static std::vector<std::pair<hitbox, rectangle>> instance{};
      return instance;
    }

    struct frame_record
    {
      double left, top, right, bottom;
      double duration;
      std::uint64_t hitbox_index;
      std::uint64_t hitbox_count;
    };
    struct hitbox_record
    {
#if defined(_DEBUG)
      std::uint64_t label_offset;
      std::uint64_t label_size;
#else
      std::uint64_t identifier;
#endif
      double left, top, right, bottom;
    };
  }

  std::span<const unsigned char> region(std::uint64_t offset, std::uint64_t size)
  { return {csp::current.base() + offset, size}; }

  std::span<const animation::frame> frames(std::size_t index, std::size_t count)
  { return {frame_storage().data() + index, count}; }

  loader::loader(const char *name, std::uint64_t signature, std::uint64_t frames_offset, std::uint64_t frames_size,
                 std::uint64_t hitboxes_offset, std::uint64_t hitboxes_size
#if defined(_DEBUG)
                 ,
                 std::uint64_t strings_offset, std::uint64_t strings_size
#endif
  )
  {
    try
    {
      const char *directory{SDL_GetBasePath()};
      if (!directory) throw exception("Failed to resolve the application directory");
      csp::mount(directory, name, signature);
      const unsigned char *base{csp::current.base()};

      auto &hitbox_pool{hitbox_storage()};
      const std::size_t hitbox_total{static_cast<std::size_t>(hitboxes_size / sizeof(hitbox_record))};
      const auto *hitbox_records{reinterpret_cast<const hitbox_record *>(base + hitboxes_offset)};
#if defined(_DEBUG)
      const auto *strings{reinterpret_cast<const char *>(base + strings_offset)};
#endif
      hitbox_pool.reserve(hitbox_total);
      for (std::size_t index{}; index < hitbox_total; ++index)
      {
        const auto &record{hitbox_records[index]};
        const rectangle bounds{record.left, record.top, record.right, record.bottom};
#if defined(_DEBUG)
        hitbox_pool.push_back({hitbox(std::string(strings + record.label_offset, record.label_size)), bounds});
#else
        hitbox_pool.push_back({hitbox(record.identifier), bounds});
#endif
      }

      auto &frame_pool{frame_storage()};
      const std::size_t frame_total{static_cast<std::size_t>(frames_size / sizeof(frame_record))};
      const auto *frame_records{reinterpret_cast<const frame_record *>(base + frames_offset)};
      frame_pool.reserve(frame_total);
      for (std::size_t index{}; index < frame_total; ++index)
      {
        const auto &record{frame_records[index]};
        std::span<const std::pair<hitbox, rectangle>> hitboxes{};
        if (record.hitbox_count)
          hitboxes = {hitbox_pool.data() + record.hitbox_index, static_cast<std::size_t>(record.hitbox_count)};
        frame_pool.push_back(
          animation::frame{rectangle{record.left, record.top, record.right, record.bottom}, record.duration, hitboxes});
      }
    }
    catch (const std::exception &error)
    {
      print<CERR>("{}.\n", error.what());
      std::exit(failure);
    }
  }
}
