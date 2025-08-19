#pragma once

#include <functional>

namespace cse::helper
{
  template <typename type> class property
  {
  public:
    property();
    property(const type &value_);
    operator type() const;
    const type *operator->() const;

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
    std::function<void()> on_change = {};

  private:
    type value = {};
  };
}

#include "property.inl"
