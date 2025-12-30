#pragma once

#include <cstddef>
#include <span>

namespace cse
{
  struct vertex
  {
    bool operator==(const vertex &other) const
    {
      return source.data() == other.source.data() && source.size() == other.source.size();
    }

    const std::span<const unsigned char> source{};
  };
  struct fragment
  {
    bool operator==(const fragment &other) const
    {
      return source.data() == other.source.data() && source.size() == other.source.size();
    }

    const std::span<const unsigned char> source{};
  };

  struct image
  {
    bool operator==(const image &other) const
    {
      return data.data() == other.data.data() && data.size() == other.data.size();
    }

    const std::span<const unsigned char> data{};
    const unsigned int width{};
    const unsigned int height{};
    const unsigned int frame_width{};
    const unsigned int frame_height{};
    const unsigned int channels{};
  };
  struct group
  {
    bool operator==(const group &other) const
    {
      return frames.data() == other.frames.data() && frames.size() == other.frames.size();
    }

    struct frame
    {
      struct rect
      {
        float top{};
        float left{};
        float bottom{};
        float right{};
      };
      const rect coords{};
      const double duration{};
    };
    std::span<const frame> frames{};
    std::size_t start{};
    std::size_t end{};
  };
  struct animation
  {
    std::size_t frame{};
    double speed{};
    bool loop{};
    double elapsed{};
  };
}
