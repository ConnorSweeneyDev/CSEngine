#pragma once

#include "property.hpp"

#include <string>

#include "exception.hpp"

namespace cse::helper
{
  template <typename type> property<type>::property(const type &value_) : value(value_) {}

  template <typename type> property<type>::operator type() const { return value; }

  template <typename type> property<type> &property<type>::operator=(const type &value_)
  {
    value = value_;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator=(const property<type> &other)
  {
    value = other.value;
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator+=(const type &value_)
  {
    if constexpr (std::is_same_v<type, std::string> || std::is_same_v<type, float> || std::is_same_v<type, int>)
      value += value_;
    else
      throw utility::exception("Unsupported operation (+=) for type '{}'", typeid(type).name());
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator++()
  {
    if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      ++value;
    else
      throw utility::exception("Unsupported operation (++) for type '{}'", typeid(type).name());
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> property<type>::operator++(int)
  {
    property<type> temp(*this);
    if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      ++value;
    else
      throw utility::exception("Unsupported operation (++) for type '{}'", typeid(type).name());
    if (on_change) on_change();
    return temp;
  }

  template <typename type> property<type> property<type>::operator+(const type &value_) const
  {
    property<type> result(*this);
    if constexpr (std::is_same_v<type, std::string> || std::is_same_v<type, float> || std::is_same_v<type, int>)
      result.value += value_;
    else
      throw utility::exception("Unsupported operation (+) for type '{}'", typeid(type).name());
    return result;
  }

  template <typename type> property<type> property<type>::operator+(const property<type> &other) const
  {
    property<type> result(*this);
    if constexpr (std::is_same_v<type, std::string> || std::is_same_v<type, float> || std::is_same_v<type, int>)
      result.value += other.value;
    else
      throw utility::exception("Unsupported operation (+) for type '{}'", typeid(type).name());
    return result;
  }

  template <typename type> property<type> &property<type>::operator-=(const type &value_)
  {
    if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      value -= value_;
    else
      throw utility::exception("Unsupported operation (-=) for type '{}'", typeid(type).name());
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> &property<type>::operator--()
  {
    if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      --value;
    else
      throw utility::exception("Unsupported operation (--) for type '{}'", typeid(type).name());
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> property<type>::operator--(int)
  {
    property<type> temp(*this);
    if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      --value;
    else
      throw utility::exception("Unsupported operation (--) for type '{}'", typeid(type).name());
    if (on_change) on_change();
    return temp;
  }

  template <typename type> property<type> property<type>::operator-(const type &value_) const
  {
    property<type> result(*this);
    if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      result.value -= value_;
    else
      throw utility::exception("Unsupported operation (-) for type '{}'", typeid(type).name());
    return result;
  }

  template <typename type> property<type> property<type>::operator-(const property<type> &other) const
  {
    property<type> result(*this);
    if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      result.value -= other.value;
    else
      throw utility::exception("Unsupported operation (-) for type '{}'", typeid(type).name());
    return result;
  }

  template <typename type> property<type> &property<type>::operator*=(const type &value_)
  {
    if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      value *= value_;
    else
      throw utility::exception("Unsupported operation (*=) for type '{}'", typeid(type).name());
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> property<type>::operator*(const type &value_) const
  {
    property<type> result(*this);
    if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      result.value *= value_;
    else
      throw utility::exception("Unsupported operation (*) for type '{}'", typeid(type).name());
    return result;
  }

  template <typename type> property<type> property<type>::operator*(const property<type> &other) const
  {
    property<type> result(*this);
    if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      result.value *= other.value;
    else
      throw utility::exception("Unsupported operation (*) for type '{}'", typeid(type).name());
    return result;
  }

  template <typename type> property<type> &property<type>::operator/=(const type &value_)
  {
    if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      value /= value_;
    else
      throw utility::exception("Unsupported operation (/=) for type '{}'", typeid(type).name());
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> property<type>::operator/(const type &value_) const
  {
    property<type> result(*this);
    if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      result.value /= value_;
    else
      throw utility::exception("Unsupported operation (/) for type '{}'", typeid(type).name());
    return result;
  }

  template <typename type> property<type> property<type>::operator/(const property<type> &other) const
  {
    property<type> result(*this);
    if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      result.value /= other.value;
    else
      throw utility::exception("Unsupported operation (/) for type '{}'", typeid(type).name());
    return result;
  }

  template <typename type> property<type> &property<type>::operator%=(const type &value_)
  {
    if constexpr (std::is_same_v<type, int>)
      value %= value_;
    else
      throw utility::exception("Unsupported operation (%=) for type '{}'", typeid(type).name());
    if (on_change) on_change();
    return *this;
  }

  template <typename type> property<type> property<type>::operator%(const type &value_) const
  {
    property<type> result(*this);
    if constexpr (std::is_same_v<type, int>)
      result.value %= value_;
    else
      throw utility::exception("Unsupported operation (%) for type '{}'", typeid(type).name());
    return result;
  }

  template <typename type> property<type> property<type>::operator%(const property<type> &other) const
  {
    property<type> result(*this);
    if constexpr (std::is_same_v<type, int>)
      result.value %= other.value;
    else
      throw utility::exception("Unsupported operation (%) for type '{}'", typeid(type).name());
    return result;
  }

  template <typename type> bool property<type>::operator==(const type &value_) const { return value == value_; }

  template <typename type> bool property<type>::operator==(const property<type> &other) const
  {
    return value == other.value;
  }

  template <typename type> bool property<type>::operator!=(const type &value_) const { return value != value_; }

  template <typename type> bool property<type>::operator!=(const property<type> &other) const
  {
    return value != other.value;
  }

  template <typename type> bool property<type>::operator<=(const type &value_) const { return value <= value_; }

  template <typename type> bool property<type>::operator<=(const property<type> &other) const
  {
    return value <= other.value;
  }

  template <typename type> bool property<type>::operator<(const type &value_) const { return value < value_; }

  template <typename type> bool property<type>::operator<(const property<type> &other) const
  {
    return value < other.value;
  }

  template <typename type> bool property<type>::operator>=(const type &value_) const { return value >= value_; }

  template <typename type> bool property<type>::operator>=(const property<type> &other) const
  {
    return value >= other.value;
  }

  template <typename type> bool property<type>::operator>(const type &value_) const { return value > value_; }

  template <typename type> bool property<type>::operator>(const property<type> &other) const
  {
    return value > other.value;
  }

  template <typename type> bool property<type>::operator!() const
  {
    if constexpr (std::is_same_v<type, bool>)
      return !value;
    else if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      return value == type{};
    else if constexpr (std::is_same_v<type, std::string>)
      return value.empty();
    else
      throw utility::exception("Unsupported operation (!) for type '{}'", typeid(type).name());
  }

  template <typename type> bool property<type>::operator&&(const property<type> &other) const
  {
    if constexpr (std::is_same_v<type, bool>)
      return value && other.value;
    else if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      return (value != type{}) && (other.value != type{});
    else if constexpr (std::is_same_v<type, std::string>)
      return !value.empty() && !other.value.empty();
    else
      throw utility::exception("Unsupported operation (&&) for type '{}'", typeid(type).name());
  }

  template <typename type> bool property<type>::operator||(const property<type> &other) const
  {
    if constexpr (std::is_same_v<type, bool>)
      return value || other.value;
    else if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      return (value != type{}) || (other.value != type{});
    else if constexpr (std::is_same_v<type, std::string>)
      return !value.empty() || !other.value.empty();
    else
      throw utility::exception("Unsupported operation (||) for type '{}'", typeid(type).name());
  }
}
