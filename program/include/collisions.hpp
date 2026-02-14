#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "glm/ext/vector_double2.hpp"

#include "declaration.hpp"
#include "hitbox.hpp"
#include "name.hpp"
#include "numeric.hpp"
#include "resource.hpp"

namespace cse
{
  struct contact
  {
    struct self
    {
      name name{};
      hitbox hitbox{};
      rectangle bounds{};
    } self;
    struct target
    {
      name name{};
      hitbox hitbox{};
      rectangle bounds{};
    } target;

    axis minimum_axis{axis::Z};
    glm::dvec2 overlap{};
    glm::dvec2 normal{};
    glm::dvec2 penetration{};
  };
};

namespace cse::help
{
  class collisions
  {
    friend class cse::object;

  public:
    template <typename callable> void handle(callable &&config) const;
    template <typename callable> void handle(const name target, callable &&config) const;
    template <typename callable> void handle(const name target, const hitbox own, callable &&config) const;
    template <typename callable>
    void handle(const name target, const hitbox own, const hitbox theirs, callable &&config) const;

  private:
    void update(const name self, const std::unordered_map<hitbox, std::shared_ptr<object>> &objects);
    void clear();

  private:
    std::unordered_map<hitbox, std::vector<contact>> contacts{};
  };
}

#include "collisions.inl" // IWYU pragma: keep
