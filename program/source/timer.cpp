#include "timer.hpp"

#include <optional>

#include "name.hpp"

namespace cse::help
{
  bool timer::has(const name name) const { return entries.contains(name); }

  bool timer::ready(const name name) const
  {
    if (auto iterator{entries.find(name)}; iterator != entries.end())
      return iterator->second.time.elapsed >= iterator->second.time.target;
    return false;
  }

  std::optional<timer::time> timer::check(const name name) const
  {
    auto iterator{entries.find(name)};
    if (iterator == entries.end()) return std::nullopt;
    return iterator->second.time;
  }

  void timer::remove(const name name) { entries.erase(name); }

  void timer::reset() noexcept { entries.clear(); }

  void timer::update(const double tick)
  {
    for (auto &[name, target] : entries) target.time.elapsed += tick;
  }
}
