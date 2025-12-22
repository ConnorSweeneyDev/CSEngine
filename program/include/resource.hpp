#pragma once

#include <algorithm>
#include <cstddef>
#include <span>
#include <string_view>
#include <utility>

#include "exception.hpp"

namespace cse::resource
{
  struct compiled_shader
  {
    const std::span<const unsigned char> source{};
    const std::size_t length{};
  };
  struct compiled_texture
  {
    struct image_data
    {
      const unsigned int width{};
      const unsigned int height{};
      const unsigned int channels{};
    };
    struct frame_data
    {
      struct group
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
        const unsigned int start_frame{};
        const unsigned int end_frame{};
        const std::span<const frame> frames{};
      };
      const group &find_group(std::string_view name) const
      {
        auto it{std::lower_bound(groups.begin(), groups.end(), name,
                                 [](const auto &pair, std::string_view key) { return pair.first < key; })};
        if (it == groups.end() || it->first != name)
          throw utility::exception("Could not find '{}' frame group for texture", name);
        return it->second;
      };
      const unsigned int width{};
      const unsigned int height{};
      const std::span<const std::pair<std::string_view, group>> groups{};
    };
    const std::span<const unsigned char> image{};
    const image_data image_data{};
    const frame_data frame_data{};
  };
}
