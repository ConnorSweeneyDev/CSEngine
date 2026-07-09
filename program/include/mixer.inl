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
  template <trait::is_audio audio> std::size_t mixer::count() const noexcept { return select<audio>().size(); }

  template <trait::is_audio audio> auto &mixer::select()
  {
    static_assert(std::is_same_v<audio, cse::sound> || std::is_same_v<audio, cse::music>, "Invalid audio type");
    if constexpr (std::is_same_v<audio, cse::sound>)
      return sounds;
    else
      return musics;
  }

  template <trait::is_audio audio> const auto &mixer::select() const
  {
    static_assert(std::is_same_v<audio, cse::sound> || std::is_same_v<audio, cse::music>, "Invalid audio type");
    if constexpr (std::is_same_v<audio, cse::sound>)
      return sounds;
    else
      return musics;
  }

  template <trait::is_audio audio> bool mixer::has(const name name) const { return select<audio>().contains(name); }

  template <trait::is_audio audio> mixer::entry<audio> &mixer::get(const name name)
  {
    auto &entries{select<audio>()};
    auto iterator{entries.find(name)};
    if (iterator == entries.end()) throw exception("Attempted to get non-existent track '{}'", name.string());
    return iterator->second;
  }

  template <trait::is_audio audio> const mixer::entry<audio> &mixer::get(const name name) const
  {
    const auto &entries{select<audio>()};
    auto iterator{entries.find(name)};
    if (iterator == entries.end()) throw exception("Attempted to get non-existent track '{}'", name.string());
    return iterator->second;
  }

  template <trait::is_audio audio> mixer::entry<audio> &mixer::set(const name name, const audio &source)
  { return select<audio>().insert_or_assign(name, entry<audio>{.source = source}).first->second; }

  template <trait::is_audio audio, typename callable> void mixer::iterate(callable &&function)
  {
    auto &entries{select<audio>()};
    std::vector<name> names{};
    names.reserve(entries.size());
    for (const auto &[name, track] : entries) names.push_back(name);
    for (const auto name : names)
      if (auto iterator{entries.find(name)}; iterator != entries.end()) function(name, iterator->second);
  }

  template <trait::is_audio audio, typename callable> void mixer::iterate(callable &&function) const
  {
    const auto &entries{select<audio>()};
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

  template <trait::is_audio audio> void mixer::remove(const name name) { select<audio>().erase(name); }

  template <trait::is_audio audio> void mixer::remove(std::initializer_list<name> names)
  {
    for (const auto name : names) select<audio>().erase(name);
  }

  template <trait::is_audio audio> void mixer::clear() noexcept { select<audio>().clear(); }
}
