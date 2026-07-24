#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "core.hpp"

namespace cse { std::shared_ptr<game> main(const std::vector<std::string_view> &arguments); }
