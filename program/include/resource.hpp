#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>

#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double4.hpp"

#include "collision.hpp"
#include "numeric.hpp"
#include "temporal.hpp"

namespace cse
{
  struct vertex
  {
    bool operator==(const vertex &other) const
    { return data.data() == other.data.data() && data.size() == other.data.size(); }

    std::span<const unsigned char> data{};
  };
  struct fragment
  {
    bool operator==(const fragment &other) const
    { return data.data() == other.data.data() && data.size() == other.data.size(); }

    std::span<const unsigned char> data{};
  };

  struct font
  {
    bool operator==(const font &other) const
    { return data.data() == other.data.data() && data.size() == other.data.size(); }

    std::span<const unsigned char> data{};
  };
  struct style
  {
    bool bold{};
    bool italic{};
    bool underline{};
    bool strikethrough{};
  };
  struct align
  {
    enum horizontal
    {
      LEFT,
      CENTER,
      RIGHT
    };
    enum vertical
    {
      TOP,
      MIDDLE,
      BOTTOM
    };
    align::horizontal horizontal{CENTER};
    align::vertical vertical{MIDDLE};
    temporal<glm::dvec2> offset{};
  };

  struct image
  {
    bool operator==(const image &other) const
    { return data.data() == other.data.data() && data.size() == other.data.size(); }

    std::span<const unsigned char> data{};
    unsigned int width{};
    unsigned int height{};
    unsigned int frame_width{};
    unsigned int frame_height{};
    unsigned int channels{};
  };
  struct animation
  {
    bool operator==(const animation &other) const
    { return frames.data() == other.frames.data() && frames.size() == other.frames.size(); }

    struct frame
    {
      const rectangle coordinates{};
      const double duration{};
      std::span<const std::pair<hitbox, rectangle>> hitboxes{};
    };
    std::span<const frame> frames{};
    std::size_t start{};
    std::size_t end{};
  };
  struct playback
  {
    std::size_t frame{};
    temporal<double> speed{};
    bool loop{};
    double elapsed{};
  };
  struct flip
  {
    bool horizontal{};
    bool vertical{};
  };
  using color = glm::dvec4;
  using transparency = double;

  struct sound
  {
    bool operator==(const sound &other) const
    { return data.data() == other.data.data() && data.size() == other.data.size(); }

    std::span<const unsigned char> data{};
  };
  struct music
  {
    bool operator==(const music &other) const
    { return data.data() == other.data.data() && data.size() == other.data.size(); }

    std::span<const unsigned char> data{};
  };
}

namespace cse::resource
{
  std::span<const unsigned char> region(std::uint64_t offset, std::uint64_t size);
  std::span<const animation::frame> frames(std::size_t index, std::size_t count);
  struct loader
  {
    loader(const char *name, std::uint64_t signature, std::uint64_t frames_offset, std::uint64_t frames_size,
           std::uint64_t hitboxes_offset, std::uint64_t hitboxes_size
#if defined(_DEBUG)
           ,
           std::uint64_t strings_offset
#endif
    );
  };
}

namespace cse::trait
{
  template <typename type>
  concept is_audio = std::is_same_v<type, sound> || std::is_same_v<type, music>;
}
