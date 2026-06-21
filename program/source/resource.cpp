#include "resource.hpp"

#include <cstdint>
#include <span>
#include <utility>
#if defined(_DEBUG)
  #include <string>
  #include <vector>
#endif

#include "collision.hpp"
#include "numeric.hpp"

namespace cse::resource
{
#if defined(_DEBUG)
  namespace { std::vector<std::pair<hitbox, rectangle>> pool{}; }
#endif

  void mount()
  {
    if (bindings.empty()) return;

    const auto base{const_cast<animation::frame *>(reinterpret_cast<const animation::frame *>(frames.data()))};
    const std::span<animation::frame> all{base, frames.size() / sizeof(animation::frame)};
    for (const auto &entry : bindings)
    {
      entry.target->frames = {base + entry.index, entry.count};
      entry.target->start = entry.start;
      entry.target->end = entry.end;
    }

#if defined(_DEBUG)
    struct record
    {
      std::uint64_t label_offset;
      std::uint64_t label_size;
      rectangle bounds;
    };
    const std::span<const record> records{reinterpret_cast<const record *>(hitboxes.data()),
                                          hitboxes.size() / sizeof(record)};
    pool.reserve(records.size());
    for (const auto &item : records)
      pool.push_back(
        {hitbox(std::string(reinterpret_cast<const char *>(strings.data()) + item.label_offset, item.label_size)),
         item.bounds});
    const auto *resolved{pool.data()};
#else
    const auto *resolved{reinterpret_cast<const std::pair<hitbox, rectangle> *>(hitboxes.data())};
#endif

    for (auto &frame : all)
    {
      if (frame.hitboxes.empty()) continue;
      const auto index{reinterpret_cast<std::uintptr_t>(frame.hitboxes.data())};
      frame.hitboxes = std::span<const std::pair<hitbox, rectangle>>{resolved + index, frame.hitboxes.size()};
    }
  }
}
