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
  template <typename tag> struct compiled_image_typed : compiled_image
  {
    constexpr compiled_image_typed() noexcept = default;
    constexpr compiled_image_typed(const std::span<const unsigned char> data_, const unsigned int width_,
                                   const unsigned int height_, const unsigned int frame_width_,
                                   const unsigned int frame_height_, const unsigned int channels_) noexcept
      : compiled_image{data_, width_, height_, frame_width_, frame_height_, channels_}
    {
    }
    constexpr compiled_image_typed(const compiled_image &image) noexcept : compiled_image(image) {}
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
  template <typename tag> struct compiled_frame_group_typed : compiled_frame_group
  {
    constexpr compiled_frame_group_typed() noexcept = default;
    constexpr compiled_frame_group_typed(unsigned int start_, unsigned int end_,
                                         std::span<const frame> frames_) noexcept
      : compiled_frame_group{start_, end_, frames_}
    {
    }
    constexpr compiled_frame_group_typed(const compiled_frame_group &group) noexcept : compiled_frame_group(group) {}
  };
}
