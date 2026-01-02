#pragma once

#include <functional>
#include <memory>
#include <type_traits>

#include "declaration.hpp"

namespace cse::help
{
  template <typename type>
  concept is_game = std::is_base_of_v<game, type>;
  template <typename type>
  concept is_window = std::is_base_of_v<window, type>;
  template <typename type>
  concept is_scene = std::is_base_of_v<scene, type>;
  template <typename type>
  concept is_camera = std::is_base_of_v<camera, type>;
  template <typename type>
  concept is_object = std::is_base_of_v<object, type>;

  template <typename type> struct function_traits;

  template <typename return_type, typename... arguments> struct function_traits<return_type(arguments...)>
  {
    using extracted_return_type = return_type;
  };

  template <typename type> struct callable_traits : callable_traits<decltype(&type::operator())>
  {
  };

  template <typename class_type, typename return_type, typename... arguments>
  struct callable_traits<return_type (class_type::*)(arguments...) const>
  {
    using signature = return_type(arguments...);
  };

  template <typename class_type, typename return_type, typename... arguments>
  struct callable_traits<return_type (class_type::*)(arguments...)>
  {
    using signature = return_type(arguments...);
  };

  template <typename return_type, typename... arguments> struct callable_traits<return_type (*)(arguments...)>
  {
    using signature = return_type(arguments...);
  };

  template <typename return_type, typename... arguments>
  struct callable_traits<std::function<return_type(arguments...)>>
  {
    using signature = return_type(arguments...);
  };

  template <typename signature> struct first_parameter_type;

  template <typename return_type, typename first_parameter, typename... rest_parameters>
  struct first_parameter_type<return_type(first_parameter, rest_parameters...)>
  {
    using type = first_parameter;
  };

  template <typename type> struct shared_ptr_inner_type
  {
    using extracted_type = type;
  };

  template <typename type> struct shared_ptr_inner_type<std::shared_ptr<type>>
  {
    using extracted_type = type;
  };

  template <typename type> struct shared_ptr_inner_type<const std::shared_ptr<type>>
  {
    using extracted_type = type;
  };

  template <typename type> struct shared_ptr_inner_type<std::shared_ptr<type> &>
  {
    using extracted_type = type;
  };

  template <typename type> struct shared_ptr_inner_type<const std::shared_ptr<type> &>
  {
    using extracted_type = type;
  };

  template <typename callable> struct type_from_callable
  {
    using signature = typename callable_traits<std::decay_t<callable>>::signature;
    using first_param = typename first_parameter_type<signature>::type;
    using clean_param = std::remove_const_t<std::remove_reference_t<first_param>>;
    using extracted_type = typename shared_ptr_inner_type<clean_param>::extracted_type;
  };
}
