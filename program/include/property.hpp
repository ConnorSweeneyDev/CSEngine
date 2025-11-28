#pragma once

#include <functional>
#include <istream>

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
    property &operator-=(const type &value_);
    property &operator*=(const type &value_);
    property &operator/=(const type &value_);
    property &operator%=(const type &value_);
    property &operator&=(const type &value_);
    property &operator|=(const type &value_);
    property &operator^=(const type &value_);
    property &operator<<=(const type &value_);
    property &operator>>=(const type &value_);
    property &operator++();
    property operator++(int);
    property &operator--();
    property operator--(int);

  public:
    std::function<void()> on_change = {};

  private:
    type value = {};
  };

  template <typename type> std::istream &operator>>(std::istream &stream_, property<type> &destination_);
}

#include "property.inl"
