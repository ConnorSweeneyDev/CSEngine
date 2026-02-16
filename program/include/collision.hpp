#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "glm/ext/vector_double2.hpp"

#include "declaration.hpp"
#include "hitbox.hpp"
#include "name.hpp"
#include "numeric.hpp"

namespace cse
{
  struct contact
  {
    struct self
    {
      class name name{};
      cse::hitbox hitbox{};
      rectangle bounds{};
    } self;
    struct target
    {
      class name name{};
      cse::hitbox hitbox{};
      rectangle bounds{};
    } target;

    axis minimum_axis{};
    glm::dvec2 overlap{};
    glm::dvec2 normal{};
    glm::dvec2 penetration{};
  };
};

namespace cse::help
{
  class collision
  {
    friend class cse::object;

  public:
    template <typename callable> void handle(callable &&config) const;

  private:
    void update(const name self, const std::unordered_map<name, std::shared_ptr<object>> &objects);
    void clear();

  private:
    std::vector<contact> contacts{};
  };
}

#include "collision.inl" // IWYU pragma: keep
