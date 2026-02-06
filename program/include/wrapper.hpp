#pragma once

#include <cstddef>
#include <format>
#include <functional>
#include <istream>
#include <optional>

namespace cse
{
  template <typename type> class temporal
  {
  public:
    temporal() = default;
    temporal(const type &value_);

  public:
    type value{};
    type rate{};
    type curve{};
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
  template <typename derived> class enumeration
  {
  public:
    class value
    {
    public:
      value();
      explicit value(int count_);

      operator int() const noexcept;

      bool operator==(const value &other_) const noexcept;
      auto operator<=>(const value &other_) const noexcept;

    private:
      int count;
    };

  protected:
    static int next(std::optional<int> count = {});
  };
}

#include "wrapper.inl" // IWYU pragma: keep
