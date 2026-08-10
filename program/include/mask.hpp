#pragma once

#include <concepts>

template <typename flags, typename mask>
concept is_maskable = requires(const flags value, const mask bits) {
  { (value & static_cast<flags>(bits)) != flags{} } -> std::convertible_to<bool>;
};

template <typename flags, typename mask>
concept is_combinable = requires(const flags value, const mask bits) {
  static_cast<flags>(value | static_cast<flags>(bits));
  static_cast<flags>(value & ~static_cast<flags>(bits));
  static_cast<flags>(value ^ static_cast<flags>(bits));
};

template <typename flags, typename mask>
  requires is_maskable<flags, mask>
constexpr bool has(const flags value, const mask bits) noexcept
{ return (value & static_cast<flags>(bits)) != flags{}; }

template <typename flags, typename... masks>
  requires(is_maskable<flags, masks> && ...)
constexpr bool any(const flags value, const masks... bits) noexcept
{ return (has(value, bits) || ...); }

template <typename flags, typename... masks>
  requires(is_maskable<flags, masks> && ...)
constexpr bool all(const flags value, const masks... bits) noexcept
{ return (has(value, bits) && ...); }

template <typename flags, typename... masks>
  requires(is_maskable<flags, masks> && ...)
constexpr bool none(const flags value, const masks... bits) noexcept
{ return (!has(value, bits) && ...); }

template <typename flags, typename... masks>
  requires(sizeof...(masks) > 0) && (is_combinable<flags, masks> && ...)
constexpr flags with(const flags value, const masks... bits) noexcept
{ return static_cast<flags>((value | ... | static_cast<flags>(bits))); }

template <typename flags, typename... masks>
  requires(sizeof...(masks) > 0) && (is_combinable<flags, masks> && ...)
constexpr flags without(const flags value, const masks... bits) noexcept
{ return static_cast<flags>((value & ... & ~static_cast<flags>(bits))); }

template <typename flags, typename... masks>
  requires(sizeof...(masks) > 0) && (is_combinable<flags, masks> && ...)
constexpr flags shared(const flags value, const masks... bits) noexcept
{ return static_cast<flags>(value & (static_cast<flags>(bits) | ...)); }

template <typename flags, typename... masks>
  requires(sizeof...(masks) > 0) && (is_combinable<flags, masks> && ...)
constexpr flags missing(const flags value, const masks... bits) noexcept
{ return static_cast<flags>((static_cast<flags>(bits) | ...) & ~value); }

template <typename flags, typename... masks>
  requires(sizeof...(masks) > 0) && (is_combinable<flags, masks> && ...)
constexpr flags toggled(const flags value, const masks... bits) noexcept
{ return static_cast<flags>(value ^ (static_cast<flags>(bits) | ...)); }
