#pragma once

#include <cstddef>
#include <span>
#include <utility>

#include "glm/ext/vector_double4.hpp"

#include "hitbox.hpp"
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
  struct rectangle
  {
    double left{};
    double top{};
    double right{};
    double bottom{};
  };
  struct animation
  {
    bool operator==(const animation &other) const
    {
      return frames.data() == other.frames.data() && frames.size() == other.frames.size();
    }

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
}
