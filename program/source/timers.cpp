#include "timers.hpp"

#include <optional>

#include "name.hpp"

namespace cse::help
{
  bool timers::has(const name name) const { return entries.contains(name); }

  bool timers::ready(const name name) const
  {
    if (auto iterator{entries.find(name)}; iterator != entries.end())
      return iterator->second.time.elapsed >= iterator->second.time.target;
    return false;
  }

  std::optional<timers::time> timers::check(const name name) const
  {
    auto iterator{entries.find(name)};
    if (iterator == entries.end()) return std::nullopt;
    return iterator->second.time;
  }

  void timers::remove(const name name) { entries.erase(name); }

  void timers::reset() noexcept { entries.clear(); }

  void timers::update(const double poll_rate)
  {
    for (auto &[name, target] : entries) target.time.elapsed += poll_rate;
  }
}
