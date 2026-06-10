#pragma once

#include <cstddef>
#include <format>
#include <functional>
#include <istream>
#include <type_traits>

namespace cse
{
  template <typename type> class property
  {
  public:
    property() = default;
    property(const type &value_);
    template <typename... arguments>
      requires(sizeof...(arguments) != 1 || (!std::is_same_v<std::decay_t<arguments>, type> && ...))
    property(arguments &&...arguments_);
    ~property() = default;
    property(const property &) = default;
    property &operator=(const property &other_);
    property(property &&) = default;
    property &operator=(property &&other_);

    operator type &() noexcept;
    operator const type &() const noexcept;
    type *operator->() noexcept;
    const type *operator->() const noexcept;
    type &operator*() noexcept;
    const type &operator*() const noexcept;

    bool operator==(const property &other_) const noexcept(noexcept(std::declval<type>() == std::declval<type>()));
    auto operator<=>(const property &other_) const noexcept(noexcept(std::declval<type>() <=> std::declval<type>()));
    bool operator==(const type &value_) const noexcept(noexcept(std::declval<type>() == std::declval<type>()));
    auto operator<=>(const type &value_) const noexcept(noexcept(std::declval<type>() <=> std::declval<type>()));

    property &operator=(const type &value_);
    template <typename other> property &operator+=(const other &value_);
    template <typename other> property &operator-=(const other &value_);
    template <typename other> property &operator*=(const other &value_);
    template <typename other> property &operator/=(const other &value_);
    template <typename other> property &operator%=(const other &value_);
    template <typename other> property &operator&=(const other &value_);
    template <typename other> property &operator|=(const other &value_);
    template <typename other> property &operator^=(const other &value_);
    template <typename other> property &operator<<=(const other &value_);
    template <typename other> property &operator>>=(const other &value_);
    property &operator++();
    property operator++(int);
    property &operator--();
    property operator--(int);

  public:
    type value{};
    std::function<void()> change{};
  };
  template <typename type> std::istream &operator>>(std::istream &stream_, property<type> &destination_);
}
template <typename type> struct std::formatter<cse::property<type>> : std::formatter<type>
{
  auto format(const cse::property<type> &property, auto &context) const;
};
template <typename type> struct std::hash<cse::property<type>>
{
  std::size_t operator()(const cse::property<type> &property) const
    noexcept(noexcept(std::hash<type>{}(std::declval<type>())));
};

#include "property.inl" // IWYU pragma: keep
