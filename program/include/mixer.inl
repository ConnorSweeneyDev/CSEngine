#pragma once

#include "mixer.hpp"

#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "name.hpp"
#include "resource.hpp"

namespace cse::help
{
  template <trait::is_audio resource> std::size_t mixer::count() const noexcept { return select<resource>().size(); }

  template <trait::is_audio resource> auto &mixer::select()
  {
    static_assert(std::is_same_v<resource, cse::sound> || std::is_same_v<resource, cse::music>,
                  "Invalid audio resource type");
    if constexpr (std::is_same_v<resource, cse::sound>)
      return sounds;
    else
      return musics;
  }

  template <trait::is_audio resource> const auto &mixer::select() const
  {
    static_assert(std::is_same_v<resource, cse::sound> || std::is_same_v<resource, cse::music>,
                  "Invalid audio resource type");
    if constexpr (std::is_same_v<resource, cse::sound>)
      return sounds;
    else
      return musics;
  }

  template <trait::is_audio resource> bool mixer::has(const name name) const
  { return select<resource>().contains(name); }

  template <trait::is_audio resource> mixer::entry<resource> &mixer::get(const name name)
  {
    auto &entries{select<resource>()};
    auto iterator{entries.find(name)};
    if (iterator == entries.end()) throw exception("Attempted to get non-existent track '{}'", name.string());
    return iterator->second;
  }

  template <trait::is_audio resource> const mixer::entry<resource> &mixer::get(const name name) const
  {
    const auto &entries{select<resource>()};
    auto iterator{entries.find(name)};
    if (iterator == entries.end()) throw exception("Attempted to get non-existent track '{}'", name.string());
    return iterator->second;
  }

  template <trait::is_audio resource> mixer::entry<resource> &mixer::set(const name name, const resource &source)
  { return select<resource>().insert_or_assign(name, entry<resource>{.source = source}).first->second; }

  template <trait::is_audio resource, typename callable> void mixer::iterate(callable &&function)
  {
    auto &entries{select<resource>()};
    std::vector<name> names{};
    names.reserve(entries.size());
    for (const auto &[name, track] : entries) names.push_back(name);
    for (const auto name : names)
      if (auto iterator{entries.find(name)}; iterator != entries.end()) function(name, iterator->second);
  }

  template <trait::is_audio resource, typename callable> void mixer::iterate(callable &&function) const
  {
    const auto &entries{select<resource>()};
    std::vector<name> names{};
    names.reserve(entries.size());
    for (const auto &[name, track] : entries) names.push_back(name);
    for (const auto name : names)
      if (auto iterator{entries.find(name)}; iterator != entries.end()) function(name, iterator->second);
  }

  template <typename callable> void mixer::iterate(callable &&function)
  {
    iterate<cse::sound>(function);
    iterate<cse::music>(std::forward<callable>(function));
  }

  template <typename callable> void mixer::iterate(callable &&function) const
  {
    iterate<cse::sound>(function);
    iterate<cse::music>(std::forward<callable>(function));
  }

  template <trait::is_audio resource> void mixer::remove(const name name) { select<resource>().erase(name); }

  template <trait::is_audio resource> void mixer::remove(std::initializer_list<name> names)
  {
    for (const auto name : names) select<resource>().erase(name);
  }

  template <trait::is_audio resource> void mixer::clear() noexcept { select<resource>().clear(); }
}
