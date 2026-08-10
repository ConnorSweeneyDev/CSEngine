#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "glm/ext/vector_double2.hpp"

#include "core.hpp"
#include "macro.hpp"
#include "name.hpp"
#include "numeric.hpp"
#include "resource.hpp"

namespace cse
{
  class collider
  {
  public:
    constexpr collider() = default;
    explicit constexpr collider(const std::uint64_t value_);

    constexpr bool operator==(const collider &other) const = default;
    constexpr collider operator|(const collider &other) const;
    constexpr collider operator&(const collider &other) const;
    constexpr collider operator^(const collider &other) const;
    constexpr collider operator~() const;
    constexpr collider &operator|=(const collider &other);
    constexpr collider &operator&=(const collider &other);
    constexpr collider &operator^=(const collider &other);

    constexpr bool empty() const;
    constexpr std::uint64_t bits() const;

  private:
    std::uint64_t value{};
  };

  struct contact
  {
    struct self
    {
      cse::name name{};
      cse::hitbox hitbox{};
    } self;
    struct target
    {
      object *pointer{};
      cse::hitbox hitbox{};
    } target;

    cse::axis axis{};
    glm::dvec2 overlap{};
    glm::dvec2 normal{};
    glm::dvec2 penetration{};
  };

  namespace help::collision
  {
    struct entry
    {
      std::int32_t left{};
      std::int32_t bottom{};
      std::int32_t right{};
      std::int32_t top{};
      std::int32_t z{};
      std::uint32_t object{};
      std::uint64_t layer{};
      std::uint64_t target{};
    };
    struct slot
    {
      std::uint32_t entry{};
      std::int32_t x{};
      std::int32_t y{};
      std::int32_t bit{};
    };

    struct store
    {
      struct registrar
      {
        registrar(const std::span<const std::string_view> colliders_);
      };

      std::span<const std::string_view> colliders{};
      bool duplicated{};
    } inline store{};

    void enlist(const std::span<const std::string_view> colliders);
    void verify();

    constexpr bool distinct(const std::span<const std::string_view> names);
    constexpr std::size_t position(const std::span<const std::string_view> names, const std::string_view label);
    template <const auto &names> constexpr cse::collider forge(const std::string_view label);
    template <const auto &names> constexpr cse::collider every();

    std::int32_t quantize(const double value);
    std::size_t cell(const std::int32_t z, const std::int32_t x, const std::int32_t y, const int bit);
    bool overlaps(const rectangle &first, const rectangle &second);
    bool overlaps(const cse::hitbox &first, const cse::hitbox &second);
    std::span<const cse::hitbox> hitboxes(const cse::object *object);
    cse::hitbox bounds(const cse::object *object, const cse::hitbox &source);
    contact describe(const name self_name, cse::object *target, const cse::hitbox &own, const cse::hitbox &theirs);
    contact mirror(const contact &source, const name self_name, cse::object *target);
    cse::hitbox hit(const cse::interface *interface, const glm::dvec2 &point);
  }
}

#define CSE_COLLIDER_STRING(collider_) #collider_,
#define CSE_COLLIDER_DECLARE(collider_)                                                                                \
  inline constexpr cse::collider collider_{cse::help::collision::forge<detail::list>(#collider_)};
#define COLLIDERS(...) CSE_JOIN(CSE_COLLIDERS_, CSE_FILLED(__VA_ARGS__))(__VA_ARGS__)
#define CSE_COLLIDERS_(...) static_assert(false, "COLLIDERS was declared without any colliders")
#define CSE_COLLIDERS_FILLED(...)                                                                                      \
  namespace collider                                                                                                   \
  {                                                                                                                    \
    namespace detail                                                                                                   \
    {                                                                                                                  \
      inline constexpr std::string_view list[]{CSE_FOR_EACH(CSE_COLLIDER_STRING, __VA_ARGS__)};                        \
      inline const cse::help::collision::store::registrar colliders{list};                                             \
    }                                                                                                                  \
    inline constexpr cse::collider none{};                                                                             \
    CSE_FOR_EACH(CSE_COLLIDER_DECLARE, __VA_ARGS__)                                                                    \
    inline constexpr cse::collider all{cse::help::collision::every<detail::list>()};                                   \
  }                                                                                                                    \
  static_assert(true)

#include "collision.inl" // IWYU pragma: keep
