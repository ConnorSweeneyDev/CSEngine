#pragma once

#include <cstddef>
#include <memory>

#include "exception.hpp"

namespace cse
{
  class type_id_generator
  {
  public:
    template <typename type> static std::size_t get() noexcept
    {
      static const std::size_t id{next_id++};
      return id;
    }

  private:
    inline static std::size_t next_id{};
  };
}

template <typename derived, typename base> bool is(const std::shared_ptr<base> &object) noexcept
{
  return object && (object->get_type_id() == cse::type_id_generator::get<derived>());
}

template <typename derived, typename base> std::shared_ptr<derived> as(const std::shared_ptr<base> &object) noexcept
{
  return std::static_pointer_cast<derived>(object);
}

template <typename derived, typename base> std::shared_ptr<derived> try_as(const std::shared_ptr<base> &object) noexcept
{
  if (is<derived>(object)) return as<derived>(object);
  return nullptr;
}

template <typename type> std::shared_ptr<type> lock(const std::weak_ptr<type> &object)
{
  if (auto locked{object.lock()}) return locked;
  throw cse::exception("Weak pointer lock failed for object");
}
