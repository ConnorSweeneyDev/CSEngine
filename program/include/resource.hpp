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
    const unsigned int channels{};
  };
  struct compiled_frame_group
  {
    struct frame
    {
      struct rect
      {
        const float top{};
        const float left{};
        const float bottom{};
        const float right{};
      };
      const rect coords{};
    };
    const unsigned int start{};
    const unsigned int end{};
    const std::span<const frame> frames{};
  };
}
