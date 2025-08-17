#pragma once

#include <functional>
#include <string>
#include <type_traits>

namespace cse::helper
{
  template <typename type> class property
  {
    static_assert(std::is_same_v<type, std::string> || std::is_same_v<type, float> || std::is_same_v<type, int> ||
                    std::is_same_v<type, bool>,
                  "The property template only supports std::string, float, int, and bool types.");

  public:
    property() = default;
    property(const type &value_);
    operator type() const;

    property &operator=(const type &value_);
    property &operator=(const property &other_);

    property &operator+=(const type &value_);
    property &operator++();
    property operator++(int);
    property operator+(const type &value_) const;
    property operator+(const property &other_) const;

    property &operator-=(const type &value_);
    property &operator--();
    property operator--(int);
    property operator-(const type &value_) const;
    property operator-(const property &other_) const;

    property &operator*=(const type &value_);
    property operator*(const type &value_) const;
    property operator*(const property &other_) const;

    property &operator/=(const type &value_);
    property operator/(const type &value_) const;
    property operator/(const property &other_) const;

    property &operator%=(const type &value_);
    property operator%(const type &value_) const;
    property operator%(const property &other_) const;

    bool operator==(const type &value_) const;
    bool operator==(const property &other_) const;
    bool operator!=(const type &value_) const;
    bool operator!=(const property &other_) const;
    bool operator<=(const type &value_) const;
    bool operator<=(const property &other_) const;
    bool operator<(const type &value_) const;
    bool operator<(const property &other_) const;
    bool operator>=(const type &value_) const;
    bool operator>=(const property &other_) const;
    bool operator>(const type &value_) const;
    bool operator>(const property &other_) const;

    bool operator!() const;
    bool operator&&(const property &other_) const;
    bool operator||(const property &other_) const;

  public:
    std::function<void()> on_change = nullptr;

  private:
    type value = {};
  };
}

#include "property.inl"
