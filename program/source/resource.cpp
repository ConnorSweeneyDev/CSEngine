#include "resource.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "SDL3/SDL_filesystem.h"
#include "csd/csd.hpp"
#include "csp/csp.hpp"

#include "exception.hpp"
#include "name.hpp"
#include "numeric.hpp"
#include "system.hpp"

namespace
{
  std::unordered_map<std::string, std::vector<cse::hitbox>> &hitbox_storage()
  {
    static std::unordered_map<std::string, std::vector<cse::hitbox>> instance{};
    return instance;
  }
  std::unordered_map<std::string, std::vector<cse::animation::frame>> &frame_storage()
  {
    static std::unordered_map<std::string, std::vector<cse::animation::frame>> instance{};
    return instance;
  }
  std::unordered_map<std::string, std::vector<cse::font::glyph>> &glyph_storage()
  {
    static std::unordered_map<std::string, std::vector<cse::font::glyph>> instance{};
    return instance;
  }
}

namespace cse::resource
{
  loader::loader(const char *name_, const std::uint64_t signature_, const std::uint64_t hitboxes_offset_,
                 const std::uint64_t hitboxes_size_, const std::uint64_t frames_offset_,
                 const std::uint64_t frames_size_, const std::uint64_t glyphs_offset_, const std::uint64_t glyphs_size_
#if defined(_DEBUG)
                 ,
                 const std::uint64_t strings_offset_
#endif
  )
  {
    try
    {
      const char *directory{SDL_GetBasePath()};
      if (!directory) throw sdl_exception("Failed to resolve the application directory");
      csp::mapping &pack{csp::mount(directory, name_, signature_)};
      const unsigned char *base{pack.base()};

      if (hitboxes_size_) csp::verify(base + hitboxes_offset_, hitboxes_size_);
      if (frames_size_) csp::verify(base + frames_offset_, frames_size_);
      if (glyphs_size_) csp::verify(base + glyphs_offset_, glyphs_size_);

      auto &hitbox_pool{hitbox_storage()[name_]};
      const std::size_t hitbox_total{static_cast<std::size_t>(hitboxes_size_ / sizeof(csd::hitbox_record))};
      const auto *hitbox_records{reinterpret_cast<const csd::hitbox_record *>(base + hitboxes_offset_)};
#if defined(_DEBUG)
      const auto *strings{reinterpret_cast<const char *>(base + strings_offset_)};
#endif
      hitbox_pool.reserve(hitbox_total);
      for (std::size_t index{}; index < hitbox_total; ++index)
      {
        const auto &record{hitbox_records[index]};
#if defined(_DEBUG)
        if (record.label_size)
          csp::verify(reinterpret_cast<const unsigned char *>(strings + record.label_offset), record.label_size);
        hitbox_pool.push_back({name(std::string(strings + record.label_offset, record.label_size)), record.left,
                               record.top, record.right, record.bottom});
#else
        hitbox_pool.push_back({name(record.identifier), record.left, record.top, record.right, record.bottom});
#endif
      }

      auto &frame_pool{frame_storage()[name_]};
      const std::size_t frame_total{static_cast<std::size_t>(frames_size_ / sizeof(csd::frame_record))};
      const auto *frame_records{reinterpret_cast<const csd::frame_record *>(base + frames_offset_)};
      frame_pool.reserve(frame_total);
      for (std::size_t index{}; index < frame_total; ++index)
      {
        const auto &record{frame_records[index]};
        std::span<const hitbox> hitboxes{};
        if (record.hitbox_count)
          hitboxes = {hitbox_pool.data() + record.hitbox_index, static_cast<std::size_t>(record.hitbox_count)};
        frame_pool.push_back(animation::frame{rectangle{record.left, record.top, record.right, record.bottom},
                                              record.duration,
                                              {record.pivot_x, record.pivot_y},
                                              hitboxes});
      }

      auto &glyph_pool{glyph_storage()[name_]};
      const std::size_t glyph_total{static_cast<std::size_t>(glyphs_size_ / sizeof(csd::glyph_record))};
      const auto *glyph_records{reinterpret_cast<const csd::glyph_record *>(base + glyphs_offset_)};
      glyph_pool.reserve(glyph_total);
      for (std::size_t index{}; index < glyph_total; ++index)
      {
        const auto &record{glyph_records[index]};
        glyph_pool.push_back(font::glyph{record.character,
                                         rectangle{record.left, record.top, record.right, record.bottom}, record.width,
                                         record.height});
      }
    }
    catch (const std::exception &error)
    {
      exception::report(error);
      std::exit(failure);
    }
  }

  std::span<const unsigned char> region(const char *pack, const std::uint64_t offset, const std::uint64_t size)
  { return {csp::mounted(pack).base() + offset, size}; }

  std::span<const animation::frame> frames(const char *pack, const std::size_t index, const std::size_t count)
  { return {frame_storage().at(pack).data() + index, count}; }

  std::span<const font::glyph> glyphs(const char *pack, const std::size_t index, const std::size_t count)
  { return {glyph_storage().at(pack).data() + index, count}; }
}
