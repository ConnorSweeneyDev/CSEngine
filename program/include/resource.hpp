#pragma once

#include <span>

namespace cse
{
  struct shader
  {
    bool operator==(const shader &other) const
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
      rect coords{};
      double duration{};
    };
    std::span<const frame> frames{};
    unsigned int start{};
    unsigned int end{};
  };
  struct animation
  {
    unsigned int frame{};
    double elapsed{};
    double speed{1.0};
  };
  struct previous
  {
    group group{};
    unsigned int frame{};
    double elapsed{};
  };
}
