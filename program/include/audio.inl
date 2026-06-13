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
      auto &track{tracks[key]};
      track.seen = true;
      if (!track.handle)
      {
        track.handle = MIX_CreateTrack(handle);
        if (!track.handle) throw sdl_exception("Could not create audio track for game");
        MIX_TagTrack(track.handle, tag);
      }

      const auto *data{entry.source.data.data()};
      const auto size{entry.source.data.size()};
      if (data != track.source || size != track.size)
      {
        track.source = data;
        track.size = size;
        track.position = entry.position;
        track.started = false;
        track.finished = false;
        track.audio = data ? require_audio(data, size, predecode) : nullptr;
        MIX_SetTrackAudio(track.handle, track.audio);
      }
      if (!data) continue;

      if (entry.loop != track.loop)
      {
        MIX_SetTrackLoops(track.handle, entry.loop ? -1 : 0);
        track.loop = entry.loop;
      }
      auto volume{entry.volume.value};
      auto speed{entry.speed.value};
      if (previous_entries)
        if (const auto iterator{previous_entries->find(entry_name)}; iterator != previous_entries->end())
        {
          volume = iterator->second.volume.value + (entry.volume.value - iterator->second.volume.value) * alpha;
          speed = iterator->second.speed.value + (entry.speed.value - iterator->second.speed.value) * alpha;
        }
      if (const auto target{bus * gain(volume)}; !equal(target, track.applied_gain))
      {
        MIX_SetTrackGain(track.handle, static_cast<float>(target));
        track.applied_gain = target;
      }
      if (speed > 0.0 && !equal(speed, track.applied_speed))
      {
        MIX_SetTrackFrequencyRatio(track.handle, static_cast<float>(speed));
        track.applied_speed = speed;
      }

      if (!equal(entry.position, track.position))
      {
        if (const auto duration{track.audio ? frames_to_seconds(MIX_GetAudioDuration(track.audio)) : 0.0};
            duration > 0.0 && entry.position >= duration)
          entry.position = entry.loop ? 0.0 : duration;
        MIX_SetTrackPlaybackPosition(track.handle, seconds_to_frames(entry.position));
        track.position = entry.position;
        track.started = false;
        track.finished = false;
      }

      if (const bool audible{entry.playing && live}; audible)
      {
        if (track.finished) {}
        else if (!track.started)
        {
          const auto options{SDL_CreateProperties()};
          SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, entry.loop ? -1 : 0);
          SDL_SetNumberProperty(options, MIX_PROP_PLAY_START_FRAME_NUMBER, seconds_to_frames(entry.position));
          MIX_PlayTrack(track.handle, options);
          SDL_DestroyProperties(options);
          track.started = true;
          track.paused = false;
        }
        else if (track.paused)
        {
          MIX_ResumeTrack(track.handle);
          track.paused = false;
        }
      }
      else if (track.started && !track.paused)
      {
        MIX_PauseTrack(track.handle);
        track.paused = true;
      }

      if (track.started && !track.paused && !track.finished)
      {
        if (const auto frames{MIX_GetTrackPlaybackPosition(track.handle)}; frames >= 0)
        {
          const auto seconds{frames_to_seconds(frames)};
          entry.position = seconds;
          track.position = seconds;
        }
        if (!entry.loop && !MIX_TrackPlaying(track.handle))
        {
          track.finished = true;
          if (track.audio)
            if (const auto duration{MIX_GetAudioDuration(track.audio)}; duration > 0)
            {
              entry.position = frames_to_seconds(duration);
              track.position = entry.position;
            }
        }
      }
    }
  }
}
