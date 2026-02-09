#pragma once

#include <cstddef>
#include <span>

#include "temporal.hpp"

namespace cse
{
  struct vertex
  {
    bool operator==(const vertex &other) const
    {
      return source.data() == other.source.data() && source.size() == other.source.size();
    }

    std::span<const unsigned char> source{};
  };
  struct fragment
  {
    bool operator==(const fragment &other) const
    {
      return source.data() == other.source.data() && source.size() == other.source.size();
    }

    std::span<const unsigned char> source{};
  };

  struct image
  {
    bool operator==(const image &other) const
    {
      return data.data() == other.data.data() && data.size() == other.data.size();
    }

    std::span<const unsigned char> data{};
    unsigned int width{};
    unsigned int height{};
    unsigned int frame_width{};
    unsigned int frame_height{};
    unsigned int channels{};
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
    temporal<double> speed{};
    bool loop{};
    double elapsed{};
  };
  struct flip
  {
    bool horizontal{};
    bool vertical{};
  };
}
