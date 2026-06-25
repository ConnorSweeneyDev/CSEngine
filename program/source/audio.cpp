#include "audio.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "SDL3/SDL_audio.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3_mixer/SDL_mixer.h"
#include "csp/csp.hpp"

#include "exception.hpp"
#include "interface.hpp"
#include "mixer.hpp"
#include "resource.hpp"
#include "scene.hpp"
#include "state.hpp"
#include "temporal.hpp"
#include "window.hpp"

namespace cse::help
{
  game_audio::game_audio(const double master_, const double sound_, const double music_)
    : previous{master_, sound_, music_}, active{master_, sound_, music_}
  {
  }

  void game_audio::prepare()
  {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) throw sdl_exception("SDL audio could not be prepared");
    if (!MIX_Init()) throw sdl_exception("SDL_mixer could not be prepared");
    if (handle = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr); !handle)
      throw sdl_exception("Could not create audio mixer for game");
    SDL_AudioSpec spec{};
    if (!MIX_GetMixerFormat(handle, &spec)) throw sdl_exception("Could not get audio mixer format for game");
    frequency = spec.freq;
  }

  void game_audio::synchronize()
  {
    previous.master = active.master;
    previous.sound = active.sound;
    previous.music = active.music;
  }

  void game_audio::mix(const help::mixer &current_previous, help::mixer &current_active,
                       const std::shared_ptr<window> &window, const std::vector<std::shared_ptr<interface>> &interfaces,
                       const std::shared_ptr<scene> &current, const double alpha)
  {
    if (!handle) return;
    active.master.value = std::clamp(active.master.value, 0.0, 1.0);
    active.sound.value = std::clamp(active.sound.value, 0.0, 1.0);
    active.music.value = std::clamp(active.music.value, 0.0, 1.0);
    const auto blend{[alpha](const temporal<double> &previous_gain, const temporal<double> &active_gain)
                     { return previous_gain.value + (active_gain.value - previous_gain.value) * alpha; }};
    const auto sound_bus{gain(blend(previous.sound, active.sound))};
    const auto music_bus{gain(blend(previous.music, active.music))};
    MIX_SetMixerGain(handle, static_cast<float>(gain(blend(previous.master, active.master))));

    std::vector<channel> channels{};
    const auto add{[&channels](auto *source)
                   { channels.push_back({&source->state.previous.mixer, &source->state.active.mixer}); }};
    channels.reserve(3 + interfaces.size());
    channels.push_back({&current_previous, &current_active});
    add(window.get());
    for (const auto &interface : interfaces) add(interface.get());
    if (current)
    {
      add(current.get());
      if (current->state.active.camera) add(current->state.active.camera.get());
      for (const auto &object : current->state.active.objects) add(object.get());
      for (const auto &interface : current->state.active.interfaces) add(interface.get());
    }

    for (auto &[key, audio] : tracks) audio.seen = false;
    for (const auto &audio : channels)
    {
      reconcile<cse::sound>(audio.previous, audio.active, alpha, "sound", true, sound_bus);
      reconcile<cse::music>(audio.previous, audio.active, alpha, "music", false, music_bus);
    }
    for (auto iterator{tracks.begin()}; iterator != tracks.end();)
      if (!iterator->second.seen)
      {
        if (iterator->second.handle) MIX_DestroyTrack(iterator->second.handle);
        iterator = tracks.erase(iterator);
      }
      else
        ++iterator;
    for (auto iterator{cache.begin()}; iterator != cache.end();)
      if (std::none_of(tracks.begin(), tracks.end(),
                       [audio = iterator->second](const auto &entry) { return entry.second.audio == audio; }))
      {
        if (iterator->second) MIX_DestroyAudio(iterator->second);
        iterator = cache.erase(iterator);
      }
      else
        ++iterator;
  }

  void game_audio::clean()
  {
    for (auto &[key, audio] : tracks)
      if (audio.handle) MIX_DestroyTrack(audio.handle);
    tracks.clear();
    for (auto &[key, audio] : cache)
      if (audio) MIX_DestroyAudio(audio);
    cache.clear();
    if (handle) MIX_DestroyMixer(handle);
    handle = nullptr;
    MIX_Quit();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
  }

  std::int64_t game_audio::seconds_to_frames(const double seconds) const
  {
    if (seconds <= 0.0 || frequency <= 0) return 0;
    return static_cast<std::int64_t>(seconds * static_cast<double>(frequency));
  }

  double game_audio::frames_to_seconds(const std::int64_t frames) const
  {
    if (frames <= 0 || frequency <= 0) return 0.0;
    return static_cast<double>(frames) / static_cast<double>(frequency);
  }

  double game_audio::gain(const double volume)
  {
    if (volume <= 0.0) return 0.0;
    return volume * volume;
  }

  MIX_Audio *game_audio::require_audio(const unsigned char *data, const std::size_t size, const bool predecode)
  {
    const track::audio_key key{data, size};
    if (const auto iterator{cache.find(key)}; iterator != cache.end()) return iterator->second;
    csp::verify(data, size);
    auto *source{SDL_IOFromConstMem(data, size)};
    if (!source) throw sdl_exception("Could not open audio data for game");
    auto *audio{MIX_LoadAudio_IO(handle, source, predecode, true)};
    if (!audio) throw sdl_exception("Could not load audio for game");
    return cache.emplace(key, audio).first->second;
  }
}
