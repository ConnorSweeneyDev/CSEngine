#pragma once

#include "hooks.hpp"

#include <functional>

#include "exception.hpp"
#include "id.hpp"

namespace cse::helper
{
  template <typename return_type, typename... arguments> bool hooks<return_type(arguments...)>::has(const id name) const
  {
    return functions.contains(name);
  }

  template <typename return_type, typename... arguments>
  void hooks<return_type(arguments...)>::add(const id name, const std::function<return_type(arguments...)> &function)
  {
    if (has(name)) throw utility::exception("Attempted to add duplicate hook");
    functions.emplace(name, function);
  }

  template <typename return_type, typename... arguments> void hooks<return_type(arguments...)>::remove(const id name)
  {
    if (!has(name)) throw utility::exception("Attempted to remove non-existent hook");
    functions.erase(name);
  }

  template <typename return_type, typename... arguments> void hooks<return_type(arguments...)>::clear() noexcept
  {
    functions.clear();
  }

  template <typename return_type, typename... arguments>
  return_type hooks<return_type(arguments...)>::call(const id name, arguments... args) const
  {
    if (!has(name)) return return_type();
    return functions.at(name)(args...);
  }

  template <typename return_type, typename... arguments>
  return_type hooks<return_type(arguments...)>::strict_call(const id name, arguments... args) const
  {
    if (!has(name)) throw utility::exception("Attempted to call required hook");
    return functions.at(name)(args...);
  }
}
