#pragma once

#include <optional>

namespace cse
{
  template <typename derived> class enumeration
  {
  public:
    class value
    {
    public:
      using is_enumeration_value = void;

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

  template <typename T>
  concept enumeration_value = requires { typename T::is_enumeration_value; };
}

#include "enumeration.inl" // IWYU pragma: keep
