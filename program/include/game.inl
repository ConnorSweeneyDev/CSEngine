#pragma once

#include "game.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include "SDL3/SDL_properties.h"
#include "SDL3_mixer/SDL_mixer.h"

#include "container.hpp"
#include "core.hpp"
#include "exception.hpp"
#include "function.hpp"
#include "interface.hpp"
#include "mixer.hpp"
#include "name.hpp"
#include "scene.hpp"

namespace cse::help::game
{
  template <typename resource> void active::reconcile_audio(const help::mixer *previous_mixer,
                                                            help::mixer *active_mixer, const char *tag,
                                                            const bool predecode, const double bus)
  {
    if (!previous_mixer) return;
    auto &entries{active_mixer->select<resource>()};
    const auto *previous_entries{previous_mixer ? &previous_mixer->select<resource>() : nullptr};
    for (auto &[entry_name, entry] : entries)
    {
      entry.volume.value = std::clamp(entry.volume.value, 0.0, 1.0);
      const audio_track::handle_key key{static_cast<const void *>(&entries), entry_name.identifier()};
      auto &audio{audio_tracks[key]};
      audio.seen = true;
      if (!audio.handle)
      {
        audio.handle = MIX_CreateTrack(audio_handle);
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
          volume = entry.volume.interpolated(iterator->second.volume, alpha);
          speed = entry.speed.interpolated(iterator->second.speed, alpha);
        }
      if (const auto target{gain(bus * volume)}; !equal(target, audio.gain))
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

      if (entry.playing)
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

namespace cse
{
  template <trait::is_game game_type, typename... game_arguments> std::shared_ptr<game_type>
  game::create(const std::function<void(const std::shared_ptr<game_type> &)> &config, game_arguments &&...arguments)
  {
    if (!instance.expired()) throw exception("Tried to create a second game instance");
    auto new_instance{std::shared_ptr<game_type>{new game_type{std::forward<game_arguments>(arguments)...}}};
    if (config) config(new_instance);
    instance = new_instance;
    return new_instance;
  }

  template <trait::is_callable callable, typename... game_arguments>
  std::shared_ptr<game> game::create(callable &&config, game_arguments &&...arguments)
  {
    using game_type = typename trait::callable_smart_inner<callable>::type;
    return create<game_type, game_arguments...>(
      std::function<void(const std::shared_ptr<game_type>)>(std::forward<callable>(config)),
      std::forward<game_arguments>(arguments)...);
  }

  template <trait::is_state state_type, typename... state_arguments>
  state_type &game::set(const name state_name, state_arguments &&...arguments)
  {
    auto state{std::make_shared<state_type>(std::forward<state_arguments>(arguments)...)};
    state->name = state_name;
    set_or_add(active.states, state);
    if (active.phase != help::phase::CREATED) set_or_add(previous.states, state);
    return *state;
  }

  template <trait::is_window window_type, typename... window_arguments>
  window_type &game::set(window_arguments &&...arguments)
  {
    auto window{std::make_shared<window_type>(std::forward<window_arguments>(arguments)...)};
    window->game = this;
    if (active.phase == help::phase::CREATED)
      next.window = window;
    else
    {
      active.window = window;
      previous.window = window;
    }
    return *window;
  }

  template <trait::is_scene scene_type, typename... scene_arguments>
  scene_type &game::set(const name scene_name, const std::function<void(const std::shared_ptr<scene_type> &)> &config,
                        scene_arguments &&...arguments)
  {
    auto scene{std::make_shared<scene_type>(std::forward<scene_arguments>(arguments)...)};
    scene->name = scene_name;
    scene->game = this;
    if (config) config(scene);
    if (auto target{try_find(active.scenes, scene_name)}; active.phase == help::phase::CREATED && target)
    {
      if (active.scene == target)
      {
        next.scene = {scene_name, scene};
        return *scene;
      }
      else
        target->clean();
    }
    set_or_add(active.scenes, scene);
    if (active.phase == help::phase::CREATED) scene->prepare();
    return *scene;
  }

  template <trait::is_callable callable, typename... scene_arguments>
  auto game::set(const name scene_name, callable &&config, scene_arguments &&...arguments) ->
    typename trait::callable_smart_inner<callable>::type &
  {
    using scene_type = typename trait::callable_smart_inner<callable>::type;
    return set<scene_type, scene_arguments...>(
      scene_name, std::function<void(const std::shared_ptr<scene_type>)>(std::forward<callable>(config)),
      std::forward<scene_arguments>(arguments)...);
  }

  template <trait::is_scene scene_type, typename... scene_arguments>
  scene_type &game::current(const name scene_name,
                            const std::function<void(const std::shared_ptr<scene_type> &)> &config,
                            scene_arguments &&...arguments)
  {
    auto scene{std::make_shared<scene_type>(std::forward<scene_arguments>(arguments)...)};
    scene->name = scene_name;
    scene->game = this;
    if (config) config(scene);
    if (active.phase == help::phase::CREATED)
      next.scene = {scene_name, scene};
    else
    {
      set_or_add(active.scenes, scene);
      active.scene = scene;
      previous.scene = scene;
    }
    return *scene;
  }

  template <trait::is_callable callable, typename... scene_arguments> trait::callable_smart_inner<callable>::type &
  game::current(const name scene_name, callable &&config, scene_arguments &&...arguments)
  {
    using scene_type = typename trait::callable_smart_inner<callable>::type;
    return current<scene_type, scene_arguments...>(
      scene_name, std::function<void(const std::shared_ptr<scene_type>)>(std::forward<callable>(config)),
      std::forward<scene_arguments>(arguments)...);
  }

  template <trait::is_interface interface_type, typename... interface_arguments>
  interface_type &game::set(const name interface_name, interface_arguments &&...arguments)
  {
    auto interface{std::make_shared<interface_type>(std::forward<interface_arguments>(arguments)...)};
    interface->name = interface_name;
    interface->game = this;
    interface->scene = std::nullopt;
    switch (active.phase)
    {
      case help::phase::CLEANED: set_or_add(active.interfaces, interface); break;
      case help::phase::PREPARED:
        if (auto existing{try_find(active.interfaces, interface_name)}) existing->clean();
        set_or_add(active.interfaces, interface);
        interface->prepare();
        break;
      case help::phase::CREATED:
        if (try_contains(active.interfaces, interface_name)) active.interface_removals.insert(interface_name);
        set_or_add(active.interface_additions, interface);
        break;
    }
    return *interface;
  }

  template <typename... target_types>
    requires((sizeof...(target_types) == 0) ||
             ((std::is_void_v<target_types> || trait::is_scene<target_types> || trait::is_interface<target_types>) &&
              ...))
  void game::remove(const name target_name)
  {
    constexpr bool all{sizeof...(target_types) == 0 || (std::is_void_v<target_types> || ...)};
    constexpr bool scenes{all || (trait::is_scene<target_types> || ...)};
    constexpr bool interfaces{all || (trait::is_interface<target_types> || ...)};
    if constexpr (scenes)
      if (auto iterator{try_iterate(active.scenes, target_name)}; iterator != active.scenes.end())
      {
        const auto &scene{*iterator};
        if (active.scene == scene || scene->active.phase == help::phase::CREATED)
          throw exception("Tried to remove current or created scene '{}'", target_name.string());
        scene->clean();
        active.scenes.erase(iterator);
      }
    if constexpr (interfaces)
      if (auto iterator{try_iterate(active.interfaces, target_name)}; iterator != active.interfaces.end())
      {
        if (auto &interface{*iterator}; active.phase == help::phase::CREATED)
          active.interface_removals.insert(target_name);
        else
        {
          if (interface->active.phase == help::phase::PREPARED) interface->clean();
          active.interfaces.erase(iterator);
        }
      }
  }
}
