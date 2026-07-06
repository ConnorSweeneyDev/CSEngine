#include "mixer.hpp"

#include <initializer_list>
#include <variant>

#include "name.hpp"

namespace cse::help
{
  bool mixer::has(const name name) const { return sounds.contains(name) || musics.contains(name); }

  void mixer::set(std::initializer_list<request> requests)
  {
    for (const auto &name : requests) std::visit([&](const auto &source) { set(name.id, source); }, name.source);
  }

  void mixer::remove(const name name)
  {
    sounds.erase(name);
    musics.erase(name);
  }

  void mixer::remove(std::initializer_list<name> names)
  {
    for (const auto name : names) remove(name);
  }

  void mixer::clear() noexcept
  {
    sounds.clear();
    musics.clear();
  }
}
