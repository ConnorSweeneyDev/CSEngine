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
    friend class cse::game;
    friend class cse::window;
    friend class cse::scene;
    friend class cse::interface;
    friend class cse::camera;
    friend class cse::object;
    friend class cse::light;
    friend struct game::active;
    friend struct window::active;
    friend struct scene::active;
    friend struct interface::active;
    friend struct camera::active;
    friend struct object::active;
    friend struct light::active;

  public:
    template <trait::is_audio audio> struct entry
    {
      audio source{};
      cse::elapsed elapsed{};
      bool playing{};
      temporal<double> speed{1.0};
      bool loop{};
      temporal<double> volume{1.0};
    };
    struct request
    {
      cse::name name{};
      std::variant<cse::sound, cse::music> source{};
    };

  public:
    mixer() = default;
    ~mixer() = default;
    mixer(const mixer &) = default;
    mixer &operator=(const mixer &other);
    mixer(mixer &&) = default;
    mixer &operator=(mixer &&) = default;

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
    template <trait::is_audio audio> void remove(const name name) noexcept;
    template <trait::is_audio audio> void remove(std::initializer_list<name> names) noexcept;
    void clear() noexcept;
    template <trait::is_audio audio> void clear() noexcept;

  private:
    void simulate(const double tick);

    template <trait::is_audio audio> auto &select() noexcept;
    template <trait::is_audio audio> const auto &select() const noexcept;

  private:
    std::unordered_map<name, entry<cse::sound>> sounds{};
    std::unordered_map<name, entry<cse::music>> musics{};
  };
}

#include "mixer.inl" // IWYU pragma: keep
