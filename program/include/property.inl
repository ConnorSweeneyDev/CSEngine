#pragma once

#include "property.hpp"

#include <cstddef>
#include <format>
#include <functional>
#include <istream>

namespace cse::helper
{
  template <typename type> property<type>::property(const type &value_) : value(value_) {}

  template <typename type> property<type> &property<type>::operator=(const property<type> &other_)
  {
    value = other_.value;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator=(property<type> &&other_)
  {
    value = std::move(other_.value);
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type>::operator type &() noexcept { return value; }

  template <typename type> property<type>::operator const type &() const noexcept { return value; }

  template <typename type> type *property<type>::operator->() noexcept { return &value; }

  template <typename type> const type *property<type>::operator->() const noexcept { return &value; }

  template <typename type> bool property<type>::operator==(const property &other_) const
    noexcept(noexcept(std::declval<type>() == std::declval<type>()))
  {
    return value == other_.value;
  }

  template <typename type> auto property<type>::operator<=>(const property &other_) const
    noexcept(noexcept(std::declval<type>() <=> std::declval<type>()))
  {
    return value <=> other_.value;
  }

  template <typename type> property<type> &property<type>::operator=(const type &value_)
  {
    value = value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator+=(const other &value_)
  {
    value += value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator-=(const other &value_)
  {
    value -= value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator*=(const other &value_)
  {
    value *= value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator/=(const other &value_)
  {
    value /= value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator%=(const other &value_)
  {
    value %= value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator&=(const other &value_)
  {
    value &= value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator|=(const other &value_)
  {
    value |= value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator^=(const other &value_)
  {
    value ^= value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator<<=(const other &value_)
  {
    value <<= value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator>>=(const other &value_)
  {
    value >>= value_;
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

  template <typename type> std::istream &operator>>(std::istream &stream_, property<type> &destination_)
  {
    type temp;
    stream_ >> temp;
    if (stream_) destination_ = temp;
    return stream_;
  }
}

template <typename type> auto
std::formatter<cse::helper::property<type>>::format(const cse::helper::property<type> &property, auto &context) const
{
  return std::formatter<type>::format(static_cast<type>(property), context);
}

template <typename type>
std::size_t std::hash<cse::helper::property<type>>::operator()(const cse::helper::property<type> &property) const
  noexcept(noexcept(std::hash<type>{}(std::declval<type>())))
{
  return std::hash<type>{}(static_cast<type>(property));
}
