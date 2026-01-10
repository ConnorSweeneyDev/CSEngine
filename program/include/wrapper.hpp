#pragma once

#include <cstddef>
#include <format>
#include <functional>
#include <istream>

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
  public:
    enumeration_value();

    operator int() const noexcept;

  private:
    int value;
  };
  template <typename derived> class enumeration
  {
    friend struct enumeration_value<derived>;

  public:
    explicit enumeration(int count_);
    ~enumeration() = default;
    enumeration(const enumeration &) = default;
    enumeration &operator=(const enumeration &) = delete;
    enumeration(enumeration &&) = default;
    enumeration &operator=(enumeration &&) = delete;

    operator int() const noexcept;

    bool operator==(const enumeration &other_) const noexcept;
    auto operator<=>(const enumeration &other_) const noexcept;

  protected:
    enumeration();

  private:
    static int &next_count();

  private:
    const int count;
  };
}

template <typename derived> struct std::hash<cse::enumeration_value<derived>>
{
  std::size_t operator()(const cse::enumeration_value<derived> &value) const noexcept;
};

template <typename derived> struct std::hash<cse::enumeration<derived>>
{
  std::size_t operator()(const cse::enumeration<derived> &enumeration) const noexcept;
};

#include "wrapper.inl" // IWYU pragma: keep
