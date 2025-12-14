#pragma once

#include <cstddef>
#include <format>
#include <functional>
#include <istream>

namespace cse::helper
{
  template <typename type> class property
  {
  public:
    property() = default;
    property(const type &value_);
    ~property() = default;
    property(const property &) = default;
    property &operator=(const property &other_);
    property(property &&) noexcept = default;
    property &operator=(property &&other_);

    operator type &() noexcept;
    operator const type &() const noexcept;
    type *operator->() noexcept;
    const type *operator->() const noexcept;

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
    std::function<void()> on_change{};

  private:
    type value{};
  };

  template <typename type> std::istream &operator>>(std::istream &stream_, property<type> &destination_);
}

template <typename type> struct std::formatter<cse::helper::property<type>> : std::formatter<type>
{
  auto format(const cse::helper::property<type> &property, auto &context) const;
};

template <typename type> struct std::hash<cse::helper::property<type>>
{
  std::size_t operator()(const cse::helper::property<type> &property) const
    noexcept(noexcept(std::hash<type>{}(std::declval<type>())));
};

#include "property.inl"
