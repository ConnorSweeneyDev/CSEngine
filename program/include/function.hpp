#pragma once

#include <functional>
#include <type_traits>

#include "pointer.hpp"

namespace cse::trait
{
  template <typename type> struct function;
  template <typename returned, typename... arguments> struct function<returned(arguments...)>
  {
    using return_type = returned;
  };

  template <typename type> struct callable : callable<decltype(&type::operator())>
  {
  };
  template <typename type, typename returned, typename... arguments>
  struct callable<returned (type::*)(arguments...) const>
  {
    using signature = returned(arguments...);
  };
  template <typename type, typename returned, typename... arguments> struct callable<returned (type::*)(arguments...)>
  {
    using signature = returned(arguments...);
  };
  template <typename returned, typename... arguments> struct callable<returned (*)(arguments...)>
  {
    using signature = returned(arguments...);
  };
  template <typename returned, typename... arguments> struct callable<std::function<returned(arguments...)>>
  {
    using signature = returned(arguments...);
  };
  template <typename signature> struct first_parameter;
  template <typename returned, typename first, typename... rest> struct first_parameter<returned(first, rest...)>
  {
    using type = first_parameter;
  };
  template <typename type>
  concept is_callable =
    std::is_function_v<std::remove_pointer_t<std::decay_t<type>>> || requires { &std::decay_t<type>::operator(); };
  template <is_callable instance> struct callable_smart_inner
  {
    using signature = typename callable<std::decay_t<instance>>::signature;
    using first = typename first_parameter<signature>::type;
    using type = typename smart_inner<std::remove_cvref_t<first>>::type;
  };
}
