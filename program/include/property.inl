#pragma once

#include "property.hpp"

#include <istream>

namespace cse::helper
{
  template <typename type> property<type>::property() : value() {}

  template <typename type> property<type>::property(const type &value_) : value(value_) {}

  template <typename type> property<type>::operator type() const { return value; }

  template <typename type> const type *property<type>::operator->() const { return &value; }

  template <typename type> property<type> &property<type>::operator=(const type &value_)
  {
    value = value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator=(const property<type> &other_)
  {
    value = other_.value;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator+=(const type &value_)
  {
    value += value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator-=(const type &value_)
  {
    value -= value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator*=(const type &value_)
  {
    value *= value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator/=(const type &value_)
  {
    value /= value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator%=(const type &value_)
  {
    value %= value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator++()
  {
    ++value;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> property<type>::operator++(int)
  {
    property<type> temp(*this);
    value++;
    if (on_change) on_change();
    return temp;
  }

  template <typename type> property<type> &property<type>::operator--()
  {
    --value;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> property<type>::operator--(int)
  {
    property<type> temp(*this);
    value--;
    if (on_change) on_change();
    return temp;
  }

  template <typename type> property<type> &property<type>::operator&=(const type &value_)
  {
    value &= value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator|=(const type &value_)
  {
    value |= value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator^=(const type &value_)
  {
    value ^= value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator<<=(const type &value_)
  {
    value <<= value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator>>=(const type &value_)
  {
    value >>= value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> std::istream &operator>>(std::istream &stream_, property<type> &destination_)
  {
    type temp;
    stream_ >> temp;
    if (stream_) destination_ = temp;
    return stream_;
  }
}
