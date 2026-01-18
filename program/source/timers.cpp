#include "timers.hpp"

#include <optional>

#include "name.hpp"

namespace cse::help
{
  bool timers::has(const help::name id) const { return entries.contains(id); }

  bool timers::ready(const help::name id) const
  {
    if (auto iterator{entries.find(id)}; iterator != entries.end())
      return iterator->second.time.elapsed >= iterator->second.time.target;
    return false;
  }

  std::optional<timers::time> timers::check(const help::name id) const
  {
    auto iterator{entries.find(id)};
    if (iterator == entries.end()) return std::nullopt;
    return iterator->second.time;
  }

  void timers::remove(const help::name id) { entries.erase(id); }

  void timers::reset() noexcept { entries.clear(); }

  void timers::update(const double poll_rate)
  {
    for (auto &[name, entry] : entries) entry.time.elapsed += poll_rate;
  }
}
