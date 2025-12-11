#pragma once

#include <functional>
#include <unordered_map>

#include "id.hpp"

namespace cse::helper
{
  template <typename type> class hooks;

  template <typename return_type, typename... arguments> class hooks<return_type(arguments...)>
  {
  public:
    bool has(const id name) const;
    void add(const id name, const std::function<return_type(arguments...)> &function);
    void remove(const id name);
    void clear() noexcept;
    return_type call(const id name, arguments... args) const;
    return_type strict_call(const id name, arguments... args) const;

  private:
    std::unordered_map<id, std::function<return_type(arguments...)>> functions = {};
  };
}

#include "hooks.inl"
