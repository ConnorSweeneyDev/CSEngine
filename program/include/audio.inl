#pragma once

#include "audio.hpp"

#include <algorithm>

#include "SDL3/SDL_properties.h"
#include "SDL3_mixer/SDL_mixer.h"

#include "exception.hpp"
#include "mixer.hpp"
#include "numeric.hpp"

namespace cse::help
{
  template <typename resource> void game_audio::reconcile(const help::mixer *previous_mix, help::mixer *active_mix,
                                                          const bool live, const double alpha, const char *tag,
                                                          const bool predecode, const double bus)
  {
    if (!active_mix) return;
    auto &entries{active_mix->select<resource>()};
    const auto *previous_entries{previous_mix ? &previous_mix->select<resource>() : nullptr};
    for (auto &[entry_name, entry] : entries)
    {
      entry.volume.value = std::clamp(entry.volume.value, 0.0, 1.0);
      const track::handle_key key{static_cast<const void *>(&entries), entry_name.identifier()};
      auto &audio{tracks[key]};
      audio.seen = true;
      if (!audio.handle)
      {
        audio.handle = MIX_CreateTrack(handle);
        if (!audio.handle) throw sdl_exception("Could not create audio track for game");
        MIX_TagTrack(audio.handle, tag);
      }

      const auto *data{entry.source.data.data()};
      const auto size{entry.source.data.size()};
      if (data != audio.source || size != audio.size)
      {
        audio.source = data;
        audio.size = size;
        audio.position = entry.position;
        audio.started = false;
        audio.finished = false;
        audio.audio = data ? require_audio(data, size, predecode) : nullptr;
        MIX_SetTrackAudio(audio.handle, audio.audio);
      }
      if (!data) continue;

      if (entry.loop != audio.loop)
      {
        MIX_SetTrackLoops(audio.handle, entry.loop ? -1 : 0);
        audio.loop = entry.loop;
      }
      auto volume{entry.volume.value};
      auto speed{entry.speed.value};
      if (previous_entries)
        if (const auto iterator{previous_entries->find(entry_name)}; iterator != previous_entries->end())
        {
          volume = iterator->second.volume.value + (entry.volume.value - iterator->second.volume.value) * alpha;
          speed = iterator->second.speed.value + (entry.speed.value - iterator->second.speed.value) * alpha;
        }
      if (const auto target{bus * gain(volume)}; !equal(target, audio.gain))
      {
        MIX_SetTrackGain(audio.handle, static_cast<float>(target));
        audio.gain = target;
      }
      if (speed > 0.0 && !equal(speed, audio.speed))
      {
        MIX_SetTrackFrequencyRatio(audio.handle, static_cast<float>(speed));
        audio.speed = speed;
      }

      if (!equal(entry.position, audio.position))
      {
        if (const auto duration{audio.audio ? frames_to_seconds(MIX_GetAudioDuration(audio.audio)) : 0.0};
            duration > 0.0 && entry.position >= duration)
          entry.position = entry.loop ? 0.0 : duration;
        MIX_SetTrackPlaybackPosition(audio.handle, seconds_to_frames(entry.position));
        audio.position = entry.position;
        audio.started = false;
        audio.finished = false;
      }

      if (const bool audible{entry.playing && live}; audible)
      {
        if (audio.finished) {}
        else if (!audio.started)
        {
          const auto options{SDL_CreateProperties()};
          SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, entry.loop ? -1 : 0);
          SDL_SetNumberProperty(options, MIX_PROP_PLAY_START_FRAME_NUMBER, seconds_to_frames(entry.position));
          MIX_PlayTrack(audio.handle, options);
          SDL_DestroyProperties(options);
          audio.started = true;
          audio.paused = false;
        }
        else if (audio.paused)
        {
          MIX_ResumeTrack(audio.handle);
          audio.paused = false;
        }
      }
      else if (audio.started && !audio.paused)
      {
        MIX_PauseTrack(audio.handle);
        audio.paused = true;
      }

      if (audio.started && !audio.paused && !audio.finished)
      {
        if (const auto frames{MIX_GetTrackPlaybackPosition(audio.handle)}; frames >= 0)
        {
          const auto seconds{frames_to_seconds(frames)};
          entry.position = seconds;
          audio.position = seconds;
        }
        if (!entry.loop && !MIX_TrackPlaying(audio.handle))
        {
          audio.finished = true;
          if (audio.audio)
            if (const auto duration{MIX_GetAudioDuration(audio.audio)}; duration > 0)
            {
              entry.position = frames_to_seconds(duration);
              audio.position = entry.position;
            }
        }
      }
    }
  }
}
