#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "SDL3_mixer/SDL_mixer.h"

#include "core.hpp"
#include "mixer.hpp"
#include "temporal.hpp"

namespace cse::help
{
  struct game_audio
  {
    friend class cse::game;

  private:
    struct channel
    {
      const help::mixer *previous{};
      help::mixer *active{};
    };
    struct track
    {
      using handle_key = std::pair<const void *, std::uint64_t>;
      using audio_key = std::pair<const unsigned char *, std::size_t>;
      MIX_Track *handle{};
      MIX_Audio *audio{};
      const unsigned char *source{};
      std::size_t size{};
      double position{};
      double gain{-1.0};
      double speed{-1.0};
      bool started{};
      bool paused{};
      bool finished{};
      bool loop{};
      bool seen{};
    };

    struct previous
    {
      temporal<double> master{0.5};
      temporal<double> sound{0.5};
      temporal<double> music{0.5};
    };
    struct active
    {
      temporal<double> master{0.5};
      temporal<double> sound{0.5};
      temporal<double> music{0.5};
    };

  public:
    game_audio() = default;
    game_audio(const double master_, const double sound_, const double music_);
    ~game_audio() = default;
    game_audio(const game_audio &) = delete;
    game_audio &operator=(const game_audio &) = delete;
    game_audio(game_audio &&) = delete;
    game_audio &operator=(game_audio &&) = delete;

  private:
    void update_previous();

    void create_app();
    void mix(const help::mixer &previous, help::mixer &active, const std::shared_ptr<window> &window,
             const std::vector<std::shared_ptr<interface>> &interfaces, const std::shared_ptr<scene> &current,
             const double alpha);
    void destroy_app();

    template <typename resource> void reconcile(const help::mixer *previous, help::mixer *active, const double alpha,
                                                const char *tag, const bool predecode, const double bus);
    std::int64_t seconds_to_frames(const double seconds) const;
    double frames_to_seconds(const std::int64_t frames) const;
    static double gain(const double volume);
    MIX_Audio *require_audio(const unsigned char *data, const std::size_t size, const bool predecode);

  public:
    game_audio::previous previous{};
    game_audio::active active{};

  private:
    int frequency{};
    MIX_Mixer *handle{};
    std::map<track::audio_key, MIX_Audio *> cache{};
    std::map<track::handle_key, track> tracks{};
  };
}

#include "audio.inl" // IWYU pragma: keep
