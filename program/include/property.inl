#pragma once

#include "property.hpp"

#include <string>

#include "exception.hpp"

namespace cse::helper
{
  template <typename type> bool is_valid_type()
  {
    return std::is_same_v<type, std::string> || std::is_same_v<type, float> || std::is_same_v<type, int> ||
           std::is_same_v<type, bool>;
  }

  template <typename type> property<type>::property()
    : value(
        []
        {
          if (!is_valid_type<type>())
            throw utility::exception("Unsupported type '{}' for property", typeid(type).name());
          return type();
        }())
  {
  }

  template <typename type> property<type>::property(const type &value_)
    : value(
        [&value_]
        {
          if (!is_valid_type<type>())
            throw utility::exception("Unsupported type '{}' for property", typeid(type).name());
          return value_;
        }())
  {
  }

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

  template <typename type> property<type> property<type>::operator+(const property<type> &other_) const
  {
    property<type> result(*this);
    if constexpr (std::is_same_v<type, std::string> || std::is_same_v<type, float> || std::is_same_v<type, int>)
      result.value += other_.value;
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

  template <typename type> property<type> property<type>::operator-(const property<type> &other_) const
  {
    property<type> result(*this);
    if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      result.value -= other_.value;
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

  template <typename type> property<type> property<type>::operator*(const property<type> &other_) const
  {
    property<type> result(*this);
    if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      result.value *= other_.value;
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

  template <typename type> property<type> property<type>::operator/(const property<type> &other_) const
  {
    property<type> result(*this);
    if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      result.value /= other_.value;
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

  template <typename type> property<type> property<type>::operator%(const property<type> &other_) const
  {
    property<type> result(*this);
    if constexpr (std::is_same_v<type, int>)
      result.value %= other_.value;
    else
      throw utility::exception("Unsupported operation (%) for type '{}'", typeid(type).name());
    return result;
  }

  template <typename type> bool property<type>::operator==(const type &value_) const { return value == value_; }

  template <typename type> bool property<type>::operator==(const property<type> &other_) const
  {
    return value == other_.value;
  }

  template <typename type> bool property<type>::operator!=(const type &value_) const { return value != value_; }

  template <typename type> bool property<type>::operator!=(const property<type> &other_) const
  {
    return value != other_.value;
  }

  template <typename type> bool property<type>::operator<=(const type &value_) const { return value <= value_; }

  template <typename type> bool property<type>::operator<=(const property<type> &other_) const
  {
    return value <= other_.value;
  }

  template <typename type> bool property<type>::operator<(const type &value_) const { return value < value_; }

  template <typename type> bool property<type>::operator<(const property<type> &other_) const
  {
    return value < other_.value;
  }

  template <typename type> bool property<type>::operator>=(const type &value_) const { return value >= value_; }

  template <typename type> bool property<type>::operator>=(const property<type> &other_) const
  {
    return value >= other_.value;
  }

  template <typename type> bool property<type>::operator>(const type &value_) const { return value > value_; }

  template <typename type> bool property<type>::operator>(const property<type> &other_) const
  {
    return value > other_.value;
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

  template <typename type> bool property<type>::operator&&(const property<type> &other_) const
  {
    if constexpr (std::is_same_v<type, bool>)
      return value && other_.value;
    else if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      return (value != type{}) && (other_.value != type{});
    else if constexpr (std::is_same_v<type, std::string>)
      return !value.empty() && !other_.value.empty();
    else
      throw utility::exception("Unsupported operation (&&) for type '{}'", typeid(type).name());
  }

  template <typename type> bool property<type>::operator||(const property<type> &other_) const
  {
    if constexpr (std::is_same_v<type, bool>)
      return value || other_.value;
    else if constexpr (std::is_same_v<type, float> || std::is_same_v<type, int>)
      return (value != type{}) || (other_.value != type{});
    else if constexpr (std::is_same_v<type, std::string>)
      return !value.empty() || !other_.value.empty();
    else
      throw utility::exception("Unsupported operation (||) for type '{}'", typeid(type).name());
  }
}
