#pragma once

#include "wrapper.hpp"

#include <cstddef>
#include <format>
#include <functional>
#include <istream>

namespace cse
{
  template <typename type> dynamic<type>::dynamic(const type &value_) : value{value_} {}
}

namespace cse
{
  template <typename type> property<type>::property(const type &value_) : value{value_} {}

  template <typename type> property<type> &property<type>::operator=(const property<type> &other_)
  {
    value = other_.value;
    if (change) change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator=(property<type> &&other_)
  {
    value = std::move(other_.value);
    if (change) change();
    return *this;
  }

  template <typename type> property<type>::operator type &() noexcept { return value; }

  template <typename type> property<type>::operator const type &() const noexcept { return value; }

  template <typename type> type *property<type>::operator->() noexcept { return &value; }

  template <typename type> const type *property<type>::operator->() const noexcept { return &value; }

  template <typename type> type &property<type>::operator*() noexcept { return value; }

  template <typename type> const type &property<type>::operator*() const noexcept { return value; }

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

  template <typename type> bool property<type>::operator==(const type &value_) const
    noexcept(noexcept(std::declval<type>() == std::declval<type>()))
  {
    return value == value_;
  }

  template <typename type> auto property<type>::operator<=>(const type &value_) const
    noexcept(noexcept(std::declval<type>() <=> std::declval<type>()))
  {
    return value <=> value_;
  }

  template <typename type> property<type> &property<type>::operator=(const type &value_)
  {
    value = value_;
    if (change) change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator+=(const other &value_)
  {
    value += value_;
    if (change) change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator-=(const other &value_)
  {
    value -= value_;
    if (change) change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator*=(const other &value_)
  {
    value *= value_;
    if (change) change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator/=(const other &value_)
  {
    value /= value_;
    if (change) change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator%=(const other &value_)
  {
    value %= value_;
    if (change) change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator&=(const other &value_)
  {
    value &= value_;
    if (change) change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator|=(const other &value_)
  {
    value |= value_;
    if (change) change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator^=(const other &value_)
  {
    value ^= value_;
    if (change) change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator<<=(const other &value_)
  {
    value <<= value_;
    if (change) change();
    return *this;
  }

  template <typename type> template <typename other> property<type> &property<type>::operator>>=(const other &value_)
  {
    value >>= value_;
    if (change) change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator++()
  {
    ++value;
    if (change) change();
    return *this;
  }

  template <typename type> property<type> property<type>::operator++(int)
  {
    property<type> temp{*this};
    value++;
    if (change) change();
    return temp;
  }

  template <typename type> property<type> &property<type>::operator--()
  {
    --value;
    if (change) change();
    return *this;
  }

  template <typename type> property<type> property<type>::operator--(int)
  {
    property<type> temp{*this};
    value--;
    if (change) change();
    return temp;
  }

  template <typename type> std::istream &operator>>(std::istream &stream_, property<type> &destination_)
  {
    type temp{};
    stream_ >> temp;
    if (stream_) destination_ = temp;
    return stream_;
  }
}

template <typename type>
auto std::formatter<cse::property<type>>::format(const cse::property<type> &property, auto &context) const
{
  return std::formatter<type>::format(static_cast<type>(property), context);
}

template <typename type>
std::size_t std::hash<cse::property<type>>::operator()(const cse::property<type> &property) const
  noexcept(noexcept(std::hash<type>{}(std::declval<type>())))
{
  return std::hash<type>{}(static_cast<type>(property));
}

namespace cse
{
  template <typename derived>
  std::size_t enumeration<derived>::value::hash::operator()(const value &value_) const noexcept
  {
    return std::hash<int>{}(static_cast<int>(value_));
  }

  template <typename derived> enumeration<derived>::value::value() : data{next()} {}

  template <typename derived> enumeration<derived>::value::operator int() const noexcept { return data; }

  template <typename derived> bool enumeration<derived>::value::operator==(const value &other_) const noexcept
  {
    return data == other_.data;
  }

  template <typename derived> auto enumeration<derived>::value::operator<=>(const value &other_) const noexcept
  {
    return data <=> other_.data;
  }

  template <typename derived> int enumeration<derived>::next()
  {
    static int counter{};
    return counter++;
  }
}
