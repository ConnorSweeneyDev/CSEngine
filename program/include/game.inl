#pragma once

#include "game.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "SDL3/SDL_properties.h"
#include "SDL3_mixer/SDL_mixer.h"
#include "glm/ext/vector_double2.hpp"

#include "container.hpp"
#include "core.hpp"
#include "exception.hpp"
#include "function.hpp"
#include "interface.hpp"
#include "mixer.hpp"
#include "name.hpp"
#include "object.hpp"
#include "resource.hpp"
#include "scene.hpp"

namespace cse::help::game
{
  template <typename first, typename second>
  std::size_t active::pair_hash::operator()(const std::pair<first, second> &key) const noexcept
  {
    const auto left{std::hash<first>{}(key.first)};
    const auto right{std::hash<second>{}(key.second)};
    return left ^ (right << 1);
  }

  template <typename type> void active::compose_text(type &text, const type &last, const cse::name &element,
                                                     const double box_left, const double box_right,
                                                     const double box_top, const double box_bottom,
                                                     std::vector<graphics_text::composed> &output)
  {
    constexpr bool is_object{std::is_same_v<type, help::object::text>};
    constexpr bool is_interface{std::is_same_v<type, help::interface::text>};
    static_assert(is_object || is_interface, "compose_text() only supports object text and interface text");
    std::string_view kind{};
    if constexpr (is_object)
      kind = "Object";
    else if constexpr (is_interface)
      kind = "Interface";
    output.clear();
    if (!usable(text.source.font.image)) throw exception("{} '{}' has text but no font", kind, element.string());
    if (text.source.font.glyphs.empty()) throw exception("Font for {} '{}' contains no glyphs", kind, element.string());
    auto &text_frame{text.playback.frame};
    const auto text_frames{text.source.animation.frames.size()};
    if (text_frames == 0) throw exception("{} '{}' text contains no frames", kind, element.string());
    if (text_frame >= text_frames) text_frame = text_frames - 1;
    const auto &text_coordinates{text.source.animation.frames[text_frame].coordinates};

    const auto text_scale{text.scale.interpolated(last.scale, alpha)};
    const auto scale_x{std::max(1.0, std::floor(text_scale.x + 0.5))};
    const auto scale_y{std::max(1.0, std::floor(text_scale.y + 0.5))};
    const auto element_width{box_right - box_left};

    constexpr std::uint32_t undefined{0xFFFD};
    const auto find{[&](const std::uint32_t character) -> const cse::font::glyph &
                    {
                      const auto locate{[&](const std::uint32_t value) -> const cse::font::glyph *
                                        {
                                          const auto position{std::ranges::lower_bound(
                                            text.source.font.glyphs, static_cast<std::uint64_t>(value),
                                            std::ranges::less{},
                                            [](const cse::font::glyph &glyph) { return glyph.character; })};
                                          if (position == text.source.font.glyphs.end() || position->character != value)
                                            return nullptr;
                                          return &*position;
                                        }};
                      if (const auto *glyph{locate(character)}) return *glyph;
                      if (const auto *glyph{locate(undefined)}) return *glyph;
                      throw exception("Font for {} '{}' is missing glyph U+{:04X} and the U+FFFD fallback glyph", kind,
                                      element.string(), character);
                    }};
    const auto &content{text.content.string()};
    const auto content_length{content.size()};
    auto &characters{graphics_text.characters};
    characters.clear();
    characters.reserve(content_length);
    for (std::size_t index{}; index < content_length;)
    {
      const auto first{static_cast<unsigned char>(content.at(index))};
      std::size_t length{1};
      std::uint32_t character{first};
      if (first >= 0xF0)
        length = 4, character = first & 0x07u;
      else if (first >= 0xE0)
        length = 3, character = first & 0x0Fu;
      else if (first >= 0xC0)
        length = 2, character = first & 0x1Fu;
      else if (first >= 0x80)
      {
        characters.push_back(undefined);
        ++index;
        continue;
      }
      if (index + length > content_length)
      {
        characters.push_back(undefined);
        break;
      }
      bool malformed{false};
      for (std::size_t offset{1}; offset < length; ++offset)
      {
        const auto continuation{static_cast<unsigned char>(content.at(index + offset))};
        if ((continuation & 0xC0u) != 0x80u)
        {
          malformed = true;
          break;
        }
        character = (character << 6u) | (continuation & 0x3Fu);
      }
      if (malformed)
      {
        characters.push_back(undefined);
        ++index;
        continue;
      }
      characters.push_back(character);
      index += length;
    }

    const auto spacing_x{text.align.horizontal.spacing.interpolated(last.align.horizontal.spacing, alpha)};
    const auto spacing_y{text.align.vertical.spacing.interpolated(last.align.vertical.spacing, alpha)};
    const auto line_height{text.source.font.glyphs.front().height * scale_y};
    auto &items{graphics_text.items};
    auto &lines{graphics_text.lines};
    auto &word{graphics_text.word};
    items.clear();
    lines.clear();
    word.clear();
    struct composer
    {
      double spacing_x{};
      double scale_x{};
      double element_width{};
      bool wrap{};
      std::vector<active::graphics_text::item> *items{};
      std::vector<active::graphics_text::line> *lines{};
      std::vector<active::graphics_text::item> *word{};
      double word_width{};

      void append(const cse::font::glyph &glyph, const std::uint32_t character)
      {
        auto &active_line{lines->back()};
        active_line.width += (active_line.count == 0 ? 0.0 : spacing_x) + (glyph.width * scale_x);
        items->push_back({&glyph, character});
        ++active_line.count;
      }
      void strip()
      {
        auto &active_line{lines->back()};
        while (active_line.count != 0 && items->back().character == U' ')
        {
          active_line.width -= items->back().glyph->width * scale_x;
          items->pop_back();
          --active_line.count;
          if (active_line.count != 0) active_line.width -= spacing_x;
        }
      }
      void commit()
      {
        strip();
        lines->push_back({.first = items->size()});
      }
      void flush()
      {
        if (word->empty()) return;
        if (wrap && lines->back().count != 0 && lines->back().width + spacing_x + word_width > element_width) commit();
        for (const auto &entry : *word)
        {
          if (wrap && lines->back().count != 0 &&
              lines->back().width + spacing_x + (entry.glyph->width * scale_x) > element_width)
            commit();
          append(*entry.glyph, entry.character);
        }
        word->clear();
        word_width = 0.0;
      }
    };
    composer compose{.spacing_x = spacing_x,
                     .scale_x = scale_x,
                     .element_width = element_width,
                     .wrap = text.overflow.wrap,
                     .items = &items,
                     .lines = &lines,
                     .word = &word};
    lines.push_back({});
    for (const auto character : characters)
    {
      if (character == U'\n')
      {
        compose.flush();
        compose.commit();
        continue;
      }
      const auto &glyph{find(character)};
      if (character == U' ')
      {
        compose.flush();
        compose.append(glyph, character);
        continue;
      }
      compose.word_width += (word.empty() ? 0.0 : spacing_x) + (glyph.width * scale_x);
      word.push_back({&glyph, character});
    }
    compose.flush();
    compose.strip();

    double block_width{};
    for (const auto &entry : lines) block_width = std::max(block_width, entry.width);
    const auto line_count{static_cast<double>(lines.size())};
    const auto block_height{(line_count * line_height) + ((line_count - 1.0) * spacing_y)};
    const glm::dvec2 shift{text.align.offset.interpolated(last.align.offset, alpha)};
    double block_left{-block_width / 2.0};
    if (text.align.horizontal.preset == LEFT)
      block_left = box_left;
    else if (text.align.horizontal.preset == RIGHT)
      block_left = box_right - block_width;
    block_left += shift.x;
    double block_top{block_height / 2.0};
    if (text.align.vertical.preset == TOP)
      block_top = box_top;
    else if (text.align.vertical.preset == BOTTOM)
      block_top = box_bottom + block_height;
    block_top += shift.y;

    for (std::size_t row{}; row < lines.size(); ++row)
    {
      const auto &entry{lines.at(row)};
      double pen{block_left};
      if (text.align.horizontal.preset == CENTER)
        pen = block_left + ((block_width - entry.width) / 2.0);
      else if (text.align.horizontal.preset == RIGHT)
        pen = block_left + (block_width - entry.width);
      const auto top{block_top - (static_cast<double>(row) * (line_height + spacing_y))};
      for (std::size_t index{entry.first}; index < entry.first + entry.count; ++index)
      {
        const auto &placed{items.at(index)};
        const auto width{placed.glyph->width * scale_x};
        const auto height{placed.glyph->height * scale_y};
        const auto left{pen};
        pen += width + spacing_x;
        if (width <= 0.0 || height <= 0.0) continue;
        auto visible_left{left};
        auto visible_right{left + width};
        auto visible_top{top};
        auto visible_bottom{top - height};
        if (text.overflow.clip)
        {
          visible_left = std::max(visible_left, box_left);
          visible_right = std::min(visible_right, box_right);
          visible_top = std::min(visible_top, box_top);
          visible_bottom = std::max(visible_bottom, box_bottom);
        }
        if (visible_right <= visible_left || visible_top <= visible_bottom) continue;
        const auto &glyph_coordinates{placed.glyph->coordinates};
        const auto uv_left{text_coordinates.left +
                           ((text_coordinates.right - text_coordinates.left) * glyph_coordinates.left)};
        const auto uv_right{text_coordinates.left +
                            ((text_coordinates.right - text_coordinates.left) * glyph_coordinates.right)};
        const auto uv_top{text_coordinates.bottom +
                          ((text_coordinates.top - text_coordinates.bottom) * glyph_coordinates.top)};
        const auto uv_bottom{text_coordinates.bottom +
                             ((text_coordinates.top - text_coordinates.bottom) * glyph_coordinates.bottom)};
        const auto fraction_left{(visible_left - left) / width};
        const auto fraction_right{(visible_right - left) / width};
        const auto fraction_top{(top - visible_top) / height};
        const auto fraction_bottom{(top - visible_bottom) / height};
        output.push_back({.left = visible_left,
                          .bottom = visible_bottom,
                          .right = visible_right,
                          .top = visible_top,
                          .uv_left = uv_left + ((uv_right - uv_left) * fraction_left),
                          .uv_bottom = uv_top + ((uv_bottom - uv_top) * fraction_bottom),
                          .uv_right = uv_left + ((uv_right - uv_left) * fraction_right),
                          .uv_top = uv_top + ((uv_bottom - uv_top) * fraction_top)});
      }
    }
  }

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
      entry.speed.value = std::abs(entry.speed.value);
      const audio_cache::track_key key{static_cast<const void *>(&entries), entry_name.identifier()};
      auto &audio{audio_cache.tracks[key]};
      audio.seen = true;
      if (!audio.handle)
      {
        audio.handle = MIX_CreateTrack(soundboard);
        if (!audio.handle) throw sdl_exception("Could not create audio track for game");
        MIX_TagTrack(audio.handle, tag);
      }

      const auto *data{entry.source.data.data()};
      const auto size{entry.source.data.size()};
      if (data != audio.source || size != audio.size)
      {
        audio.source = data;
        audio.size = size;
        audio.position = entry.elapsed.device;
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
      if (const auto target{bus * volume}; !equal(target, audio.gain))
      {
        MIX_SetTrackGain(audio.handle, static_cast<float>(target));
        audio.gain = target;
      }
      if (speed > 0.0 && !equal(speed, audio.speed))
      {
        MIX_SetTrackFrequencyRatio(audio.handle, static_cast<float>(speed));
        audio.speed = speed;
      }

      if (!equal(entry.elapsed.device, audio.position))
      {
        if (const auto duration{audio.audio ? frames_to_seconds(MIX_GetAudioDuration(audio.audio)) : 0.0};
            duration > 0.0 && entry.elapsed.device >= duration)
          entry.elapsed.device = entry.loop ? 0.0 : duration;
        MIX_SetTrackPlaybackPosition(audio.handle, seconds_to_frames(entry.elapsed.device));
        audio.position = entry.elapsed.device;
        audio.started = false;
        audio.finished = false;
      }

      const auto running{entry.playing && entry.speed.value > 0.0};
      if (running)
      {
        if (audio.finished) {}
        else if (!audio.started)
        {
          const auto options{SDL_CreateProperties()};
          SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, entry.loop ? -1 : 0);
          SDL_SetNumberProperty(options, MIX_PROP_PLAY_START_FRAME_NUMBER, seconds_to_frames(entry.elapsed.device));
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
          entry.elapsed.device = seconds;
          audio.position = seconds;
        }
        if (!entry.loop && !MIX_TrackPlaying(audio.handle))
        {
          audio.finished = true;
          if (audio.audio)
            if (const auto duration{MIX_GetAudioDuration(audio.audio)}; duration > 0)
            {
              entry.elapsed.device = frames_to_seconds(duration);
              audio.position = entry.elapsed.device;
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
    if (instance) throw exception("Tried to create a second game instance");
    auto new_instance{std::shared_ptr<game_type>{new game_type{std::forward<game_arguments>(arguments)...}}};
    if (config) config(new_instance);
    instance = new_instance;
    return new_instance;
  }

  template <trait::is_callable callable, typename... game_arguments>
  std::shared_ptr<game> game::create(callable &&config, game_arguments &&...arguments)
  {
    using game_type = trait::callable_smart_inner<callable>::type;
    return create<game_type, game_arguments...>(
      std::function<void(const std::shared_ptr<game_type>)>(std::forward<callable>(config)),
      std::forward<game_arguments>(arguments)...);
  }

  template <trait::is_state state_type, typename... state_arguments>
  state_type &game::set(const name state_name, state_arguments &&...arguments)
  {
    auto state{std::make_shared<state_type>(std::forward<state_arguments>(arguments)...)};
    state->name = state_name;
    active.states.set(state);
    if (active.phase != help::phase::CREATED) previous.states.set(state);
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
    if (auto target{active.scenes.find(scene_name)}; active.phase == help::phase::CREATED && target)
    {
      if (active.scene == target)
      {
        next.scene = {scene_name, scene};
        return *scene;
      }
      else
        target->clean();
    }
    active.scenes.set(scene);
    if (active.phase == help::phase::CREATED) scene->prepare();
    return *scene;
  }

  template <trait::is_callable callable, typename... scene_arguments>
  auto game::set(const name scene_name, callable &&config, scene_arguments &&...arguments)
    -> trait::callable_smart_inner<callable>::type &
  {
    using scene_type = trait::callable_smart_inner<callable>::type;
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
      active.scenes.set(scene);
      active.scene = scene;
    }
    return *scene;
  }

  template <trait::is_callable callable, typename... scene_arguments> trait::callable_smart_inner<callable>::type &
  game::current(const name scene_name, callable &&config, scene_arguments &&...arguments)
  {
    using scene_type = trait::callable_smart_inner<callable>::type;
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
      case help::phase::CLEANED: active.interfaces.set(interface); break;
      case help::phase::PREPARED:
        if (auto existing{active.interfaces.find(interface_name)}) existing->clean();
        active.interfaces.set(interface);
        interface->prepare();
        break;
      case help::phase::CREATED:
        if (active.interfaces.contains(interface_name)) active.interface_removals.insert(interface_name);
        active.interface_additions.set(interface);
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
      if (auto scene{active.scenes.find(target_name)})
      {
        if (active.scene == scene || scene->active.phase == help::phase::CREATED)
          throw exception("Tried to remove current or created scene '{}'", target_name.string());
        scene->clean();
        active.scenes.remove(target_name);
      }
    if constexpr (interfaces)
      if (auto interface{active.interfaces.find(target_name)})
      {
        if (active.phase == help::phase::CREATED)
          active.interface_removals.insert(target_name);
        else
        {
          if (interface->active.phase == help::phase::PREPARED) interface->clean();
          active.interfaces.remove(target_name);
        }
      }
  }
}
