#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "core.hpp"
#include "meta.hpp"

namespace cse
{
  struct application
  {
    std::shared_ptr<cse::game> instance{};
    cse::meta::initial meta{};
  };
  using arguments = std::vector<std::string_view>;

  application main(const arguments &arguments);
}
