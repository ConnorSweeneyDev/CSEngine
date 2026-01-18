#include "timers.hpp"

#include <functional>
#include <optional>

#include "exception.hpp"
#include "name.hpp"

namespace cse::help
{
  bool timers::has(const help::name id) const { return entries.contains(id); }

  bool timers::ready(const help::name id) const
  {
    if (auto it{entries.find(id)}; it != entries.end()) return it->second.time.elapsed >= it->second.time.target;
    return false;
  }

  std::optional<timers::time> timers::check(const help::name id) const
  {
    if (auto it{entries.find(id)}; it != entries.end()) return it->second.time;
    return std::nullopt;
  }

  void timers::remove(const help::name id) { entries.erase(id); }

  void timers::reset() noexcept { entries.clear(); }

  bool timers::call(const help::name id)
  {
    auto it{entries.find(id)};
    if (it == entries.end() || !(it->second.time.elapsed >= it->second.time.target)) return false;
    if (it->second.callback) it->second.callback();
    entries.erase(it);
    return true;
  }

  void timers::throw_call(const help::name id)
  {
    auto it{entries.find(id)};
    if (it == entries.end()) throw exception("Tried to call timer that does not exist");
    if (!(it->second.time.elapsed >= it->second.time.target)) throw exception("Tried to call timer that is not ready");
    if (!it->second.callback) throw exception("Tried to call timer with no callback");
    it->second.callback();
    entries.erase(it);
  }

  void timers::update(const double poll_rate)
  {
    for (auto &[name, entry] : entries) entry.time.elapsed += poll_rate;
  }
}
