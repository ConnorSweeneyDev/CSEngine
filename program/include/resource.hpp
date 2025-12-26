#pragma once

#include <cstddef>
#include <span>

namespace cse
{
  struct compiled_shader
  {
    const std::span<const unsigned char> source{};
    const std::size_t length{};
  };

  struct compiled_image
  {
    const std::span<const unsigned char> data{};
    const unsigned int width{};
    const unsigned int height{};
    const unsigned int frame_width{};
    const unsigned int frame_height{};
    const unsigned int channels{};
  };
  struct compiled_frame_group
  {
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
    };
    unsigned int start{};
    unsigned int end{};
    std::span<const frame> frames{};
  };
}
