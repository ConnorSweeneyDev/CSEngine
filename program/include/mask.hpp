#pragma once

#include <type_traits>

template <typename flags, typename mask>
  requires std::is_integral_v<flags>
inline bool has(const flags value, const mask bits) noexcept
{ return (value & static_cast<flags>(bits)) != 0; }

template <typename flags, typename... masks>
  requires std::is_integral_v<flags>
inline bool any(const flags value, const masks... bits) noexcept
{ return (has(value, bits) || ...); }

template <typename flags, typename... masks>
  requires std::is_integral_v<flags>
inline bool all(const flags value, const masks... bits) noexcept
{ return (has(value, bits) && ...); }

template <typename flags, typename... masks>
  requires std::is_integral_v<flags>
inline bool none(const flags value, const masks... bits) noexcept
{ return (!has(value, bits) && ...); }
