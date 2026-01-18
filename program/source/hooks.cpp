#include "hooks.hpp"

namespace cse::help
{
  bool hooks::has(const int key) const { return entries.contains(key); }

  void hooks::remove(const int key) { entries.erase(key); }

  void hooks::reset() noexcept { entries.clear(); }
}
