#pragma once

#include <memory>

template <typename derived, typename base> std::shared_ptr<derived> as(const std::shared_ptr<base> &object);
template <typename type> std::shared_ptr<type> lock(const std::weak_ptr<type> &object);

#include "utility.inl"
