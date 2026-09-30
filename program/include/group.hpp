#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "macro.hpp"

namespace cse
{
  class group
  {
  public:
    static constexpr std::size_t capacity{64};
    using word = std::array<std::uint64_t, capacity / 64>;

  public:
    constexpr group() = default;
    explicit constexpr group(const word &value_);

    constexpr bool operator==(const group &other) const = default;
    constexpr group operator|(const group &other) const;
    constexpr group operator&(const group &other) const;
    constexpr group operator^(const group &other) const;
    constexpr group operator~() const;
    constexpr group &operator|=(const group &other);
    constexpr group &operator&=(const group &other);
    constexpr group &operator^=(const group &other);

    constexpr bool empty() const;
    constexpr const word &words() const;

  private:
    word value{};
  };

  namespace help::group
  {
    struct store
    {
      struct registrar { registrar(const std::span<const std::string_view> tags_); };

      std::span<const std::string_view> tags{};
      bool duplicated{};
    } inline store{};

    void enlist(const std::span<const std::string_view> tags);
    void verify();

    constexpr bool distinct(const std::span<const std::string_view> names);
    constexpr bool fits(const std::span<const std::string_view> names);
    constexpr std::size_t position(const std::span<const std::string_view> names, const std::string_view label);
    constexpr bool member(const std::string_view tag, const std::string_view owner);
    constexpr cse::group single(const std::size_t index);
    template <const auto &names> constexpr cse::group forge(const std::string_view label);
    template <const auto &names> constexpr cse::group every(const std::string_view owner);

    constexpr int lowest(const cse::group &value);
    constexpr std::size_t count(const cse::group &value);
    template <typename action> constexpr void visit(const cse::group &value, const action &perform);
  }
}

#define CSE_GROUP_NAME(tuple_) CSE_GROUP_NAME_ tuple_
#define CSE_GROUP_NAME_(group_, ...) #group_,
#define CSE_GROUP_STRINGS(tuple_) CSE_GROUP_STRINGS_ tuple_
#define CSE_GROUP_STRINGS_(group_, ...) CSE_FOR_EACH_WITH(CSE_GROUP_STRING, group_, __VA_ARGS__)
#define CSE_GROUP_STRING(group_, tag_) #group_ "." #tag_,
#define CSE_GROUP_NAMESPACE(tuple_) CSE_GROUP_NAMESPACE_ tuple_
#define CSE_GROUP_NAMESPACE_(group_, ...)                                                                              \
  namespace group_                                                                                                     \
  {                                                                                                                    \
    CSE_FOR_EACH_WITH(CSE_GROUP_DECLARE, group_, __VA_ARGS__)                                                          \
    inline constexpr cse::group all{cse::help::group::every<group_detail::tags>(#group_)};                             \
    static_assert(!all.empty(), "GROUPS declares the group '" #group_ "' without any tags");                           \
  }
#define CSE_GROUP_DECLARE(group_, tag_)                                                                                \
  inline constexpr cse::group tag_{cse::help::group::forge<group_detail::tags>(#group_ "." #tag_)};
#define GROUPS(...) CSE_JOIN(CSE_GROUPS_, CSE_FILLED(__VA_ARGS__))(__VA_ARGS__)
#define CSE_GROUPS_(...) static_assert(false, "GROUPS was declared without any groups")
#define CSE_GROUPS_FILLED(...)                                                                                         \
  namespace group_detail                                                                                               \
  {                                                                                                                    \
    inline constexpr std::string_view groups[]{CSE_FOR_EACH(CSE_GROUP_NAME, __VA_ARGS__)};                             \
    inline constexpr std::string_view tags[]{CSE_FOR_EACH(CSE_GROUP_STRINGS, __VA_ARGS__)};                            \
    static_assert(cse::help::group::distinct(groups), "GROUPS declares the same group more than once");                \
    static_assert(cse::help::group::distinct(tags), "GROUPS declares the same tag more than once in one group");       \
    static_assert(cse::help::group::fits(tags), "GROUPS declares more than 64 tags in total");                         \
    inline const cse::help::group::store::registrar registrar{tags};                                                   \
  }                                                                                                                    \
  CSE_FOR_EACH(CSE_GROUP_NAMESPACE, __VA_ARGS__)                                                                       \
  static_assert(true)

#include "group.inl" // IWYU pragma: export
