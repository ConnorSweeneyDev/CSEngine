#include "timer.hpp"

#include <cstddef>
#include <initializer_list>
#include <unordered_map>

#include "exception.hpp"
#include "name.hpp"

namespace cse::help
{
  std::size_t timer::count() const noexcept { return entries.size(); }

  bool timer::has(const name name) const { return entries.contains(name); }

  timer::state &timer::get(const name name)
  {
    auto iterator{entries.find(name)};
    if (iterator == entries.end()) throw exception("Attempted to get non-existent timer '{}'", name.string());
    return iterator->second.state;
  }

  const timer::state &timer::get(const name name) const
  {
    auto iterator{entries.find(name)};
    if (iterator == entries.end()) throw exception("Attempted to get non-existent timer '{}'", name.string());
    return iterator->second.state;
  }

  void timer::remove(const name name) { entries.erase(name); }

  void timer::remove(std::initializer_list<name> names)
  {
    for (const auto &name : names) entries.erase(name);
  }

  void timer::clear() noexcept { entries.clear(); }

  bool timer::ready(const name name) const
  {
    if (auto iterator{entries.find(name)}; iterator != entries.end())
      return iterator->second.state.elapsed >= iterator->second.state.target;
    return false;
  }

  void timer::finish(typename std::unordered_map<name, entry>::iterator iterator)
  {
    if (iterator->second.state.repeat)
      iterator->second.state.elapsed = 0.0;
    else
      entries.erase(iterator);
  }

  void timer::update(const double tick)
  {
    for (auto &[name, target] : entries)
      if (target.state.running) target.state.elapsed += tick;
  }
}
