#pragma once

#include "container.hpp"

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <utility>

#include "exception.hpp"
#include "name.hpp"
#include "pointer.hpp"

namespace cse::help
{
  template <typename type> container<type>::container(const container &other) : elements{other.elements} {};

  template <typename type> auto container<type>::begin() const noexcept -> typename storage::const_iterator
  { return elements.begin(); }

  template <typename type> auto container<type>::end() const noexcept -> typename storage::const_iterator
  { return elements.end(); }

  template <typename type> std::size_t container<type>::size() const noexcept { return elements.size(); }

  template <typename type> bool container<type>::empty() const noexcept { return elements.empty(); }

  template <typename type>
  const typename container<type>::element &container<type>::at(const std::size_t position) const
  { return elements.at(position); }

  template <typename type> typename container<type>::element container<type>::operator[](const cse::name name) const
  {
    const auto *value{locate(name)};
    if (!value) throw exception("Could not find element with name: {}", name.string());
    return *value;
  }

  template <typename type> typename container<type>::element container<type>::find(const cse::name name) const noexcept
  {
    const auto *value{locate(name)};
    if (!value) return nullptr;
    return *value;
  }

  template <typename type> bool container<type>::contains(const cse::name name) const noexcept
  { return locate(name) != nullptr; }

  template <typename type> template <typename... derived> bool container<type>::is(const cse::name name) const
  { return ::is<derived...>((*this)[name]); }

  template <typename type> template <typename... derived>
  bool container<type>::try_is(const cse::name name) const noexcept
  { return ::try_is<derived...>(find(name)); }

  template <typename type> template <typename... derived> bool container<type>::is_a(const cse::name name) const
  { return ::is_a<derived...>((*this)[name]); }

  template <typename type> template <typename... derived>
  bool container<type>::try_is_a(const cse::name name) const noexcept
  { return ::try_is_a<derived...>(find(name)); }

  template <typename type> template <typename derived>
  std::shared_ptr<derived> container<type>::as(const cse::name name) const
  { return ::as<derived>((*this)[name]); }

  template <typename type> template <typename derived>
  std::shared_ptr<derived> container<type>::try_as(const cse::name name) const noexcept
  { return ::try_as<derived>(find(name)); }

  template <typename type> template <typename derived>
  std::shared_ptr<derived> container<type>::as_a(const cse::name name) const
  { return ::as_a<derived>((*this)[name]); }

  template <typename type> template <typename derived>
  std::shared_ptr<derived> container<type>::try_as_a(const cse::name name) const noexcept
  { return ::try_as_a<derived>(find(name)); }

  template <typename type> container<type> &container<type>::operator=(const container &other)
  {
    if (this == &other) return *this;
    elements = other.elements;
    index.clear();
    indexed = false;
    return *this;
  }

  template <typename type> container<type>::container(container &&other) noexcept
    : elements{std::move(other.elements)}, index{std::move(other.index)}, indexed{other.indexed}
  {
    other.index.clear();
    other.indexed = false;
  };

  template <typename type> container<type> &container<type>::operator=(container &&other) noexcept
  {
    if (this == &other) return *this;
    elements = std::move(other.elements);
    index = std::move(other.index);
    indexed = other.indexed;
    other.index.clear();
    other.indexed = false;
    return *this;
  }

  template <typename type> void container<type>::set(const element &value)
  {
    if (!value) return;
    if (!indexed) reindex();
    if (const auto iterator{index.find(value->name)}; iterator != index.end())
    {
      elements.at(iterator->second) = value;
      return;
    }
    elements.push_back(value);
    index.insert_or_assign(value->name, elements.size() - 1);
  }

  template <typename type> bool container<type>::remove(const cse::name name)
  {
    if (!indexed) reindex();
    const auto iterator{index.find(name)};
    if (iterator == index.end()) return false;
    elements.erase(elements.begin() + static_cast<std::ptrdiff_t>(iterator->second));
    index.clear();
    indexed = false;
    return true;
  }

  template <typename type> void container<type>::clear() noexcept
  {
    elements.clear();
    index.clear();
    indexed = true;
  }

  template <typename type> void container<type>::reindex() const
  {
    index.clear();
    index.reserve(elements.size());
    for (std::size_t position{}; position < elements.size(); ++position)
      if (const auto &value{elements.at(position)}) index.insert_or_assign(value->name, position);
    indexed = true;
  }

  template <typename type>
  const typename container<type>::element *container<type>::locate(const cse::name name) const noexcept
  {
    if (!indexed) reindex();
    const auto iterator{index.find(name)};
    if (iterator == index.end()) return nullptr;
    return &elements.at(iterator->second);
  }
}
