#include "mixer.hpp"

#include <initializer_list>
#include <variant>

#include "name.hpp"

namespace cse::help
{
  void mixer::load(std::initializer_list<request> requests)
  {
    for (const auto &request : requests)
      std::visit([&](const auto &source) { load(request.id, source); }, request.source);
  }

  void mixer::unload(const name name)
  {
    sounds.erase(name);
    musics.erase(name);
  }

  void mixer::clear() noexcept
  {
    sounds.clear();
    musics.clear();
  }
}
