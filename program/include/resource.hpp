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

  struct align
  {
    struct horizontal_axis
    {
      ::horizontal preset{CENTER};
      temporal<double> spacing{};
    };
    struct vertical_axis
    {
      ::vertical preset{MIDDLE};
      temporal<double> spacing{};
    };
    horizontal_axis horizontal{};
    vertical_axis vertical{};
    temporal<glm::dvec2> offset{};
  };
  struct overflow
  {
    bool wrap{};
    bool clip{true};
  };
  struct color
  {
    temporal<glm::dvec4> tint{{0.5, 0.5, 0.5, 1.0}};
    temporal<double> alpha{1.0};
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
  struct font
  {
    bool operator==(const font &other) const
    { return image == other.image && glyphs.data() == other.glyphs.data() && glyphs.size() == other.glyphs.size(); }

    struct glyph
    {
      std::uint64_t character{};
      rectangle coordinates{};
      double width{};
      double height{};
    };
    cse::image image{};
    std::span<const glyph> glyphs{};
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
    temporal<double> speed{1.0};
    bool loop{true};
    double elapsed{};
  };
  struct flip
  {
    bool horizontal{};
    bool vertical{};
  };

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
  struct loader
  {
    loader(const char *name_, const std::uint64_t signature_, const std::uint64_t frames_offset_,
           const std::uint64_t frames_size_, const std::uint64_t hitboxes_offset_, const std::uint64_t hitboxes_size_,
           const std::uint64_t glyphs_offset_, const std::uint64_t glyphs_size_
#if defined(_DEBUG)
           ,
           const std::uint64_t strings_offset_
#endif
    );
  };

  std::span<const unsigned char> region(const char *pack, const std::uint64_t offset, const std::uint64_t size);
  std::span<const animation::frame> frames(const char *pack, const std::size_t index, const std::size_t count);
  std::span<const font::glyph> glyphs(const char *pack, const std::size_t index, const std::size_t count);
}

namespace cse::trait
{
  template <typename type>
  concept is_shader = std::is_same_v<type, vertex> || std::is_same_v<type, fragment>;
  template <typename type>
  concept is_audio = std::is_same_v<type, sound> || std::is_same_v<type, music>;
}
