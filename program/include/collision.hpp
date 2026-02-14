#pragma once

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "declaration.hpp"
#include "name.hpp"

namespace cse::help
{
  class collisions
  {
    friend class cse::object;

  public:
    template <typename callable> void check(const name other, callable &&config) const;
    bool hit(const name target, const name own, const name theirs) const;

  private:
    void update(const name self, const std::unordered_map<name, std::shared_ptr<object>> &objects);
    void clear();

  private:
    std::unordered_map<name, std::vector<std::pair<name, name>>> entries{};
  };
}

#include "collision.inl" // IWYU pragma: keep
