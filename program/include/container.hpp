#pragma once

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

#include "core.hpp"
#include "name.hpp"

namespace cse::help
{
  template <typename type> class container
  {
    friend class cse::game;
    friend class cse::scene;
    friend struct game::active;
    friend struct scene::active;

  public:
    using element = std::shared_ptr<type>;
    using storage = std::vector<element>;

  public:
    container() = default;
    ~container() = default;
    container(const container &other);

    auto begin() const noexcept -> typename storage::const_iterator;
    auto end() const noexcept -> typename storage::const_iterator;
    std::size_t size() const noexcept;
    bool empty() const noexcept;

    const element &at(const std::size_t position) const;
    element operator[](const cse::name name) const;
    element find(const cse::name name) const noexcept;
    bool contains(const cse::name name) const noexcept;

  private:
    container &operator=(const container &other);
    container(container &&other) noexcept;
    container &operator=(container &&other) noexcept;

    void set(const element &value);
    bool remove(const cse::name name);
    void clear() noexcept;

    void reindex() const;
    const element *locate(const cse::name name) const noexcept;

  private:
    storage elements{};
    mutable std::unordered_map<cse::name, std::size_t> index{};
    mutable bool indexed{};
  };
}

#include "container.inl" // IWYU pragma: keep
