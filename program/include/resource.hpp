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
  template <typename Tag> struct compiled_image_typed : compiled_image
  {
    using compiled_image::compiled_image;
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
  template <typename Tag> struct compiled_frame_group_typed : compiled_frame_group
  {
    using compiled_frame_group::compiled_frame_group;
    constexpr compiled_frame_group_typed(const compiled_frame_group &group) noexcept : compiled_frame_group(group) {}
  };
}
