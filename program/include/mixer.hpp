#pragma once

#include <initializer_list>
#include <unordered_map>
#include <variant>

#include "core.hpp"
#include "name.hpp"
#include "resource.hpp"
#include "temporal.hpp"

namespace cse::help
{
  class mixer
  {
    friend struct game::active;
    friend struct window::active;
    friend struct scene::active;
    friend struct camera::active;
    friend struct object::active;
    friend struct interface::active;

  public:
    template <trait::is_audio resource> struct entry
    {
      resource source{};
      double position{};
      bool playing{};
      bool loop{};
      temporal<double> speed{1.0};
      temporal<double> volume{0.5};
    };
    struct request
    {
      request(const name id_, const cse::sound &source_) : id{id_}, source{source_} {}
      request(const name id_, const cse::music &source_) : id{id_}, source{source_} {}
      name id{};
      std::variant<cse::sound, cse::music> source{};
    };

  public:
    template <trait::is_audio resource> bool has(const name name) const;
    template <trait::is_audio resource> entry<resource> &get(const name name);
    template <trait::is_audio resource> const entry<resource> &get(const name name) const;
    template <trait::is_audio resource> entry<resource> &load(const name name, const resource &source);
    void load(std::initializer_list<request> requests);
    template <trait::is_audio resource> void unload(const name name);
    void unload(const name name);
    template <trait::is_audio resource> void clear() noexcept;
    void clear() noexcept;

  private:
    template <trait::is_audio resource> auto &select();
    template <trait::is_audio resource> const auto &select() const;

  private:
    std::unordered_map<name, entry<cse::sound>> sounds{};
    std::unordered_map<name, entry<cse::music>> musics{};
  };
}

#include "mixer.inl" // IWYU pragma: keep
