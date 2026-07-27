#include "mixer.hpp"

#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <unordered_map>
#include <variant>

#include "name.hpp"

namespace cse::help
{
  mixer &mixer::operator=(const mixer &other)
  {
    if (this == &other) return *this;
    const auto reconcile{[](const auto &source, auto &target)
                         {
                           std::erase_if(target, [&source](const auto &item) { return !source.contains(item.first); });
                           for (const auto &[name, track] : source)
                             if (const auto iterator{target.find(name)}; iterator != target.end())
                               iterator->second = track;
                             else
                               target.emplace(name, track);
                         }};
    reconcile(other.sounds, sounds);
    reconcile(other.musics, musics);
    return *this;
  }

  std::size_t mixer::count() const noexcept { return sounds.size() + musics.size(); }

  bool mixer::has(const name name) const { return sounds.contains(name) || musics.contains(name); }

  void mixer::set(std::initializer_list<request> requests)
  {
    for (const auto &item : requests) std::visit([&](const auto &source) { set(item.name, source); }, item.source);
  }

  void mixer::remove(const name name)
  {
    sounds.erase(name);
    musics.erase(name);
  }

  void mixer::remove(std::initializer_list<name> names)
  {
    for (const auto &name : names) remove(name);
  }

  void mixer::clear() noexcept
  {
    sounds.clear();
    musics.clear();
  }

  void mixer::simulate(const double tick)
  {
    const auto step{[tick](auto &entries)
                    {
                      for (auto &[name, track] : entries)
                      {
                        track.speed.value = std::abs(track.speed.value);
                        if (!track.playing || track.speed.value <= 0.0) continue;
                        const auto duration{track.source.duration};
                        if (duration > 0.0 && !track.loop && track.elapsed.tick >= duration) continue;
                        track.elapsed.tick += tick * track.speed.value;
                        if (duration <= 0.0) continue;
                        if (track.loop)
                          track.elapsed.tick = std::fmod(track.elapsed.tick, duration);
                        else if (track.elapsed.tick > duration)
                          track.elapsed.tick = duration;
                      }
                    }};
    step(sounds);
    step(musics);
  }
}
