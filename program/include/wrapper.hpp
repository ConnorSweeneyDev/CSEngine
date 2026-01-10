#pragma once

#include <cstddef>
#include <format>
#include <functional>
#include <istream>
#include <type_traits>

namespace cse
{
  template <typename type> class dynamic
  {
  public:
    dynamic() = default;
    dynamic(const type &value_);

  public:
    type value{};
    type velocity{};
    type acceleration{};
  };
}

namespace cse
{
  template <typename type> class property
  {
  public:
    property() = default;
    property(const type &value_);
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
    std::function<void()> change{};

  private:
    type value{};
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

namespace cse
{
  template <typename derived> struct enumeration_value
  {
    static_assert(std::is_same_v<derived, typename derived::domain_type>,
                  "enumeration_value must use the base domain type, not a derived type");

  public:
    enumeration_value();

    operator int() const noexcept;

    bool operator==(const enumeration_value &other_) const noexcept;
    auto operator<=>(const enumeration_value &other_) const noexcept;

  private:
    int value;
  };

  template <typename derived> class enumeration
  {
    friend struct enumeration_value<derived>;

  public:
    using domain_type = derived;

  protected:
    static int next();
  };
}

template <typename derived> struct std::hash<cse::enumeration_value<derived>>
{
  std::size_t operator()(const cse::enumeration_value<derived> &value_) const noexcept;
};

#include "wrapper.inl" // IWYU pragma: keep
