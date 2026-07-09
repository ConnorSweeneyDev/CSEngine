#pragma once

#include <cstddef>
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
    friend struct light::active;
    friend struct interface::active;

  public:
    template <trait::is_audio audio> struct entry
    {
      audio source{};
      double position{};
      bool loop{};
      temporal<double> speed{1.0};
      temporal<double> volume{0.5};
      bool playing{};
    };
    struct request
    {
      request(const name id_, const cse::sound &source_) : id{id_}, source{source_} {}
      request(const name id_, const cse::music &source_) : id{id_}, source{source_} {}
      name id{};
      std::variant<cse::sound, cse::music> source{};
    };

  public:
    std::size_t count() const noexcept;
    template <trait::is_audio audio> std::size_t count() const noexcept;
    bool has(const name name) const;
    template <trait::is_audio audio> bool has(const name name) const;
    template <trait::is_audio audio> entry<audio> &get(const name name);
    template <trait::is_audio audio> const entry<audio> &get(const name name) const;
    template <trait::is_audio audio> entry<audio> &set(const name name, const audio &source);
    void set(std::initializer_list<request> requests);
    template <trait::is_audio audio, typename callable> void iterate(callable &&function);
    template <trait::is_audio audio, typename callable> void iterate(callable &&function) const;
    template <typename callable> void iterate(callable &&function);
    template <typename callable> void iterate(callable &&function) const;
    void remove(const name name);
    void remove(std::initializer_list<name> names);
    template <trait::is_audio audio> void remove(const name name);
    template <trait::is_audio audio> void remove(std::initializer_list<name> names);
    void clear() noexcept;
    template <trait::is_audio audio> void clear() noexcept;

  private:
    template <trait::is_audio audio> auto &select();
    template <trait::is_audio audio> const auto &select() const;

  private:
    std::unordered_map<name, entry<cse::sound>> sounds{};
    std::unordered_map<name, entry<cse::music>> musics{};
  };
}

#include "mixer.inl" // IWYU pragma: keep
