#include "game.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "SDL3/SDL_audio.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_timer.h"
#include "SDL3_mixer/SDL_mixer.h"
#include "csp/csp.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/trigonometric.hpp"

#include "collision.hpp"
#include "container.hpp"
#include "core.hpp"
#include "exception.hpp"
#include "interface.hpp"
#include "light.hpp"
#include "log.hpp"
#include "mask.hpp"
#include "mixer.hpp"
#include "name.hpp"
#include "numeric.hpp"
#include "object.hpp"
#include "resource.hpp"
#include "shader.hpp"
#include "system.hpp"
#include "temporal.hpp"
#include "window.hpp"

namespace cse::help::game
{
  previous::previous(const double tick_, const double frame_, const temporal<double> &aspect_,
                     const unsigned int resolution_, const temporal<glm::dvec3> &clear_,
                     const temporal<double> &master_, const temporal<double> &sound_, const temporal<double> &music_)
    : tick(tick_), frame(frame_), aspect(aspect_), resolution(resolution_), clear(clear_), master(master_),
      sound(sound_), music(music_)
  {
  }

  active::active(const double tick_, const double frame_, const temporal<double> &aspect_,
                 const unsigned int resolution_, const temporal<glm::dvec3> &clear_, const temporal<double> &master_,
                 const temporal<double> &sound_, const temporal<double> &music_)
    : tick(tick_), frame(frame_), aspect(aspect_), resolution(resolution_), clear(clear_), master(master_),
      sound(sound_), music(music_)
  {
  }

  void active::prepare()
  {
    SDL_SetLogPriorities(debug ? SDL_LOG_PRIORITY_DEBUG : SDL_LOG_PRIORITY_ERROR);
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game"))
      sdl_log("Could not set app metadata type");
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING, "Connor.Sweeney.Engine"))
      sdl_log("Could not set app metadata identifier");
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "CSEngine"))
      sdl_log("Could not set app metadata name");
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, "1.0.0"))
      sdl_log("Could not set app metadata version");
    if (!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "Connor Sweeney"))
      sdl_log("Could not set app metadata creator");

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) throw sdl_exception("SDL could not be prepared");
    audio_ready = false;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
      sdl_log("SDL audio could not be prepared; continuing without sound");
    else if (!MIX_Init())
      sdl_log("SDL_mixer could not be prepared; continuing without sound");
    else
      audio_ready = true;
  }

  void active::create()
  {
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, debug, "vulkan");
    if (!device) throw sdl_exception("Could not create GPU device");

    const std::array<corner, 4> vertices{
      {{1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, -1.0f, 1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f, 1.0f}, {-1.0f, -1.0f, 0.0f, 0.0f}}};
    const std::array<Uint16, 6> indices{3, 1, 0, 3, 0, 2};
    SDL_GPUBufferCreateInfo vertex_buffer_info{
      .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = sizeof(vertices), .props = 0};
    graphics_buffer.vertex = SDL_CreateGPUBuffer(device, &vertex_buffer_info);
    if (!graphics_buffer.vertex) throw sdl_exception("Could not create vertex buffer for game");
    SDL_GPUBufferCreateInfo index_buffer_info{.usage = SDL_GPU_BUFFERUSAGE_INDEX, .size = sizeof(indices), .props = 0};
    graphics_buffer.index = SDL_CreateGPUBuffer(device, &index_buffer_info);
    if (!graphics_buffer.index) throw sdl_exception("Could not create index buffer for game");
    SDL_GPUSamplerCreateInfo sampler_info{};
    sampler_info.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    graphics_buffer.nearest = SDL_CreateGPUSampler(device, &sampler_info);
    if (!graphics_buffer.nearest) throw sdl_exception("Could not create nearest sampler for game");
    SDL_GPUSamplerCreateInfo linear_sampler_info{sampler_info};
    linear_sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
    linear_sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
    graphics_buffer.linear = SDL_CreateGPUSampler(device, &linear_sampler_info);
    if (!graphics_buffer.linear) throw sdl_exception("Could not create linear sampler for game");
    graphics_light.capacity = 16;
    SDL_GPUBufferCreateInfo light_buffer_info{
      .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
      .size = static_cast<Uint32>(sizeof(graphics_light::entry) * graphics_light.capacity),
      .props = 0};
    graphics_light.buffer = SDL_CreateGPUBuffer(device, &light_buffer_info);
    if (!graphics_light.buffer) throw sdl_exception("Could not create light storage buffer for game");
    SDL_GPUTransferBufferCreateInfo light_transfer_buffer_info{
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = static_cast<Uint32>(sizeof(graphics_light::entry) * graphics_light.capacity),
      .props = 0};
    graphics_light.transfer_buffer = SDL_CreateGPUTransferBuffer(device, &light_transfer_buffer_info);
    if (!graphics_light.transfer_buffer) throw sdl_exception("Could not create light transfer buffer for game");
    graphics_occluder.capacity = 16;
    SDL_GPUBufferCreateInfo occluder_buffer_info{
      .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
      .size = static_cast<Uint32>(sizeof(graphics_occluder::entry) * graphics_occluder.capacity),
      .props = 0};
    graphics_occluder.buffer = SDL_CreateGPUBuffer(device, &occluder_buffer_info);
    if (!graphics_occluder.buffer) throw sdl_exception("Could not create occluder storage buffer for game");
    SDL_GPUTransferBufferCreateInfo occluder_transfer_buffer_info{
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = static_cast<Uint32>(sizeof(graphics_occluder::entry) * graphics_occluder.capacity),
      .props = 0};
    graphics_occluder.transfer_buffer = SDL_CreateGPUTransferBuffer(device, &occluder_transfer_buffer_info);
    if (!graphics_occluder.transfer_buffer) throw sdl_exception("Could not create occluder transfer buffer for game");
    SDL_GPUTextureCreateInfo occluder_array_info{.type = SDL_GPU_TEXTURETYPE_2D_ARRAY,
                                                 .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                                 .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
                                                 .width = 1,
                                                 .height = 1,
                                                 .layer_count_or_depth = 1,
                                                 .num_levels = 1,
                                                 .sample_count = SDL_GPU_SAMPLECOUNT_1,
                                                 .props = 0};
    graphics_occluder.texture = SDL_CreateGPUTexture(device, &occluder_array_info);
    if (!graphics_occluder.texture) throw sdl_exception("Could not create occluder texture array for game");
    graphics_occluder.width = 1;
    graphics_occluder.height = 1;
    graphics_occluder.layers = 1;
    SDL_GPUTransferBufferCreateInfo transfer_buffer_info{
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = sizeof(vertices) + sizeof(indices), .props = 0};
    auto *transfer_buffer{SDL_CreateGPUTransferBuffer(device, &transfer_buffer_info)};
    if (!transfer_buffer) throw sdl_exception("Could not create transfer buffer for game");
    auto start{static_cast<char *>(SDL_MapGPUTransferBuffer(device, transfer_buffer, false))};
    if (!start) throw sdl_exception("Could not map data for game");
    SDL_memcpy(start, vertices.data(), sizeof(vertices));
    SDL_memcpy(start + sizeof(vertices), indices.data(), sizeof(indices));
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
    auto *command_buffer{SDL_AcquireGPUCommandBuffer(device)};
    if (!command_buffer) throw sdl_exception("Could not acquire GPU command buffer for game");
    auto *copy_pass{SDL_BeginGPUCopyPass(command_buffer)};
    if (!copy_pass) throw sdl_exception("Could not begin GPU copy pass for game");
    SDL_GPUTransferBufferLocation vertex_transfer_location{.transfer_buffer = transfer_buffer, .offset = 0};
    SDL_GPUBufferRegion vertex_buffer_region{.buffer = graphics_buffer.vertex, .offset = 0, .size = sizeof(vertices)};
    SDL_UploadToGPUBuffer(copy_pass, &vertex_transfer_location, &vertex_buffer_region, false);
    SDL_GPUTransferBufferLocation index_transfer_location{.transfer_buffer = transfer_buffer,
                                                          .offset = sizeof(vertices)};
    SDL_GPUBufferRegion index_buffer_region{.buffer = graphics_buffer.index, .offset = 0, .size = sizeof(indices)};
    SDL_UploadToGPUBuffer(copy_pass, &index_transfer_location, &index_buffer_region, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

    if (!audio_ready) return;
    if (audio_handle = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr); !audio_handle)
    {
      sdl_log("Could not create audio mixer for game; continuing without sound");
      return;
    }
    SDL_AudioSpec spec{};
    if (!MIX_GetMixerFormat(audio_handle, &spec))
    {
      sdl_log("Could not get audio mixer format for game; continuing without sound");
      MIX_DestroyMixer(audio_handle);
      audio_handle = nullptr;
      return;
    }
    frequency = spec.freq;
  }

  void active::synchronize(previous &last)
  {
    last.tick = tick;
    last.frame = frame;
    last.aspect = aspect;
    last.resolution = resolution;
    last.clear = clear;
    last.master = master;
    last.sound = sound;
    last.music = music;

    last.states = states;
    last.window = window;
    last.scenes = scenes;
    last.scene = scene;
    last.interfaces = interfaces;
    last.timer = timer;
    last.mixer = mixer;
    last.phase = phase;

    for (auto &[name, track] : mixer.sounds)
    {
      track.speed.instant = false;
      track.volume.instant = false;
    }
    for (auto &[name, track] : mixer.musics)
    {
      track.speed.instant = false;
      track.volume.instant = false;
    }
    aspect.instant = false;
    clear.instant = false;
    master.instant = false;
    sound.instant = false;
    music.instant = false;
  }

  void active::render(const temporal<double> previous_aspect)
  {
    graphics_light.data.meta[0] = 0.0f;
    graphics_light.data.meta[1] = 0.0f;
    generate_graphics_order();
    generate_interfaces();
    draw_batches(
      [&]()
      {
        const auto height{static_cast<double>(std::max(1u, resolution))};
        const auto real_aspect{aspect.interpolated(previous_aspect, alpha)};
        const auto width{height * real_aspect};
        const auto projection{glm::ortho(-width / 2.0, width / 2.0, -height / 2.0, height / 2.0, -1.0, 1.0)};
        const glm::dvec3 origin{std::llround(width) % 2 == 0 ? -0.5 : 0.0, std::llround(height) % 2 == 0 ? 0.5 : 0.0,
                                0.0};
        return std::pair<glm::dmat4, glm::dmat4>{projection, glm::translate(glm::dmat4{1.0}, origin)};
      }());
    for (auto iterator{graphics_cache.texture.begin()}; iterator != graphics_cache.texture.end();)
      if (iterator->second.stamp != graphics_cache.stamp)
      {
        SDL_ReleaseGPUTexture(device, iterator->second.value);
        iterator = graphics_cache.texture.erase(iterator);
      }
      else
        ++iterator;
    graphics_cache.stamp++;
  }

  void active::mix(const help::mixer &previous_mixer, const temporal<double> previous_master,
                   const temporal<double> previous_sound, const temporal<double> previous_music)
  {
    if (!audio_handle) return;
    master.value = std::clamp(master.value, 0.0, 1.0);
    sound.value = std::clamp(sound.value, 0.0, 1.0);
    music.value = std::clamp(music.value, 0.0, 1.0);
    const auto blend{[this](const temporal<double> &previous_gain, const temporal<double> &active_gain)
                     { return active_gain.interpolated(previous_gain, alpha); }};
    const auto master_bus{blend(previous_master, master)};
    const auto sound_bus{master_bus * blend(previous_sound, sound)};
    const auto music_bus{master_bus * blend(previous_music, music)};

    std::vector<channel> channels{};
    const auto add{[&channels](auto *source) { channels.push_back({&source->previous.mixer, &source->active.mixer}); }};
    channels.reserve(3 + interfaces.size());
    channels.push_back({&previous_mixer, &mixer});
    add(window.get());
    if (scene)
    {
      add(scene.get());
      for (const auto &element : scene->active.interfaces) add(element.get());
      if (scene->active.camera) add(scene->active.camera.get());
      for (const auto &element : scene->active.objects) add(element.get());
      for (const auto &element : scene->active.lights) add(element.get());
    }
    for (const auto &element : interfaces) add(element.get());

    for (auto &[key, audio] : audio_tracks) audio.seen = false;
    for (const auto &audio : channels)
    {
      reconcile_audio<cse::sound>(audio.previous, audio.active, "sound", true, sound_bus);
      reconcile_audio<cse::music>(audio.previous, audio.active, "music", false, music_bus);
    }
    for (auto iterator{audio_tracks.begin()}; iterator != audio_tracks.end();)
      if (!iterator->second.seen)
      {
        if (iterator->second.handle) MIX_DestroyTrack(iterator->second.handle);
        iterator = audio_tracks.erase(iterator);
      }
      else
        ++iterator;
    for (auto iterator{audio_cache.begin()}; iterator != audio_cache.end();)
      if (std::none_of(audio_tracks.begin(), audio_tracks.end(),
                       [audio = iterator->second](const auto &entry) { return entry.second.audio == audio; }))
      {
        if (iterator->second) MIX_DestroyAudio(iterator->second);
        iterator = audio_cache.erase(iterator);
      }
      else
        ++iterator;
  }

  void active::destroy()
  {
    for (auto &[key, audio] : audio_tracks)
      if (audio.handle) MIX_DestroyTrack(audio.handle);
    audio_tracks.clear();
    for (auto &[key, audio] : audio_cache)
      if (audio) MIX_DestroyAudio(audio);
    audio_cache.clear();
    if (audio_handle) MIX_DestroyMixer(audio_handle);
    audio_handle = nullptr;

    SDL_ReleaseGPUTransferBuffer(device, graphics_object.transfer_buffer);
    SDL_ReleaseGPUBuffer(device, graphics_object.buffer);
    SDL_ReleaseGPUTransferBuffer(device, graphics_light.transfer_buffer);
    SDL_ReleaseGPUBuffer(device, graphics_light.buffer);
    SDL_ReleaseGPUTransferBuffer(device, graphics_occluder.transfer_buffer);
    SDL_ReleaseGPUBuffer(device, graphics_occluder.buffer);
    SDL_ReleaseGPUTexture(device, graphics_occluder.texture);
    for (const auto &[key, texture] : graphics_cache.texture) SDL_ReleaseGPUTexture(device, texture.value);
    SDL_ReleaseGPUGraphicsPipeline(device, graphics_pipeline.interface);
    SDL_ReleaseGPUGraphicsPipeline(device, graphics_pipeline.transparent);
    SDL_ReleaseGPUGraphicsPipeline(device, graphics_pipeline.opaque);
    SDL_ReleaseGPUSampler(device, graphics_buffer.linear);
    SDL_ReleaseGPUSampler(device, graphics_buffer.nearest);
    SDL_ReleaseGPUBuffer(device, graphics_buffer.index);
    SDL_ReleaseGPUBuffer(device, graphics_buffer.vertex);
    SDL_DestroyGPUDevice(device);
    graphics_interface.order.clear();
    graphics_object.transfer_buffer = nullptr;
    graphics_object.buffer = nullptr;
    graphics_object.capacity = 0;
    graphics_object.samples.clear();
    graphics_object.batches.clear();
    graphics_light.transfer_buffer = nullptr;
    graphics_light.buffer = nullptr;
    graphics_light.capacity = 0;
    graphics_light.samples.clear();
    graphics_occluder.transfer_buffer = nullptr;
    graphics_occluder.buffer = nullptr;
    graphics_occluder.texture = nullptr;
    graphics_occluder.capacity = 0;
    graphics_occluder.width = 0;
    graphics_occluder.height = 0;
    graphics_occluder.layers = 0;
    graphics_occluder.samples.clear();
    graphics_occluder.signature.clear();
    graphics_cache.texture.clear();
    graphics_cache.stamp = 0;
    graphics_pipeline.interface = nullptr;
    graphics_pipeline.transparent = nullptr;
    graphics_pipeline.opaque = nullptr;
    graphics_buffer.linear = nullptr;
    graphics_buffer.nearest = nullptr;
    graphics_buffer.index = nullptr;
    graphics_buffer.vertex = nullptr;
    device = nullptr;
  }

  void active::clean()
  {
    MIX_Quit();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Quit();
  }

  void active::generate_simulation_order()
  {
    interface_order.clear();
    for (interface_order.reserve(interfaces.size()); const auto &element : interfaces)
      interface_order.emplace_back(element.get());
    std::sort(interface_order.begin(), interface_order.end(),
              [](const cse::interface *left, const cse::interface *right)
              {
                if (left->active.priority.simulation != right->active.priority.simulation)
                  return left->active.priority.simulation > right->active.priority.simulation;
                return left->name.identifier() < right->name.identifier();
              });
  }

  void active::generate_pool()
  {
    interface_pool.clear();
    interface_pool.reserve(scene->active.interface_simulation_order.size() + interface_order.size());
    interface_pool.insert(interface_pool.end(), scene->active.interface_simulation_order.begin(),
                          scene->active.interface_simulation_order.end());
    interface_pool.insert(interface_pool.end(), interface_order.begin(), interface_order.end());
    std::sort(interface_pool.begin(), interface_pool.end(),
              [](const cse::interface *left, const cse::interface *right)
              {
                if (left->active.priority.simulation != right->active.priority.simulation)
                  return left->active.priority.simulation > right->active.priority.simulation;
                if (const auto left_layer{left->scene ? 0 : 1}, right_layer{right->scene ? 0 : 1};
                    left_layer != right_layer)
                  return left_layer > right_layer;
                return left->name.identifier() < right->name.identifier();
              });
  }

  bool active::inside(const glm::dvec2 &position)
  {
    const auto canvas_height{static_cast<double>(std::max(1u, resolution))};
    const auto canvas_width{canvas_height * aspect.value};
    const auto x{position.x - (std::llround(canvas_width) % 2 == 0 ? 0.5 : 0.0)};
    const auto y{position.y + (std::llround(canvas_height) % 2 == 0 ? 0.5 : 0.0)};
    return x >= -canvas_width / 2.0 && x < canvas_width / 2.0 && y > -canvas_height / 2.0 && y <= canvas_height / 2.0;
  }

  void active::interact()
  {
    const auto &event{window->active.event};
    const auto &mouse{window->active.mouse};
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
      if (event.button.button > SDL_BUTTON_X2 || !inside(mouse.position)) return;
      for (auto *element : interface_pool)
        if (const auto target{collision::hit(element, mouse.position)}; target != hitbox{})
        {
          element->active.target.pressed[event.button.button] = target;
          break;
        }
    }
    else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
      if (event.button.button > SDL_BUTTON_X2) return;
      for (auto *element : interface_pool)
        if (const auto target{element->active.target.pressed[event.button.button]}; target != hitbox{})
        {
          element->active.target.clicked[event.button.button] = target;
          element->active.target.pressed[event.button.button] = {};
        }
    }
  }

  void active::hover()
  {
    std::optional<glm::dvec2> position{};
    const auto &mouse{window->active.mouse};
    if (SDL_GetMouseFocus() == window->active.instance && inside(mouse.position)) position = mouse.position;
    cse::interface *hit{};
    hitbox target{};
    if (position)
      for (auto *element : interface_pool)
        if (target = collision::hit(element, *position); target != hitbox{})
        {
          hit = element;
          break;
        }
    for (auto *element : interface_pool) element->active.target.hovered = element == hit ? target : hitbox{};
  }

  void active::generate_graphics_order()
  {
    const auto comparator{[](const cse::interface *left, const cse::interface *right)
                          {
                            if (left->active.priority.rendering != right->active.priority.rendering)
                              return left->active.priority.rendering < right->active.priority.rendering;
                            if (const auto left_layer{left->scene ? 0 : 1}, right_layer{right->scene ? 0 : 1};
                                left_layer != right_layer)
                              return left_layer < right_layer;
                            if (const auto *left_batch{left->active.texture.image.data.data()},
                                *right_batch{right->active.texture.image.data.data()};
                                left_batch != right_batch)
                              return left_batch < right_batch;
                            return left->name.identifier() < right->name.identifier();
                          }};
    graphics_interface.order.clear();
    graphics_interface.order.reserve(scene->active.interfaces.size() + interfaces.size());
    for (const auto &element : scene->active.interfaces) graphics_interface.order.emplace_back(element.get());
    for (const auto &element : interfaces) graphics_interface.order.emplace_back(element.get());
    std::sort(graphics_interface.order.begin(), graphics_interface.order.end(), comparator);
  }

  void active::generate_interfaces()
  {
    graphics_object.samples.clear();
    graphics_object.batches.clear();
    graphics_object.samples.reserve(graphics_interface.order.size());
    for (auto *element : graphics_interface.order)
    {
      auto &current{element->active.texture.playback.frame};
      const auto size{element->active.texture.animation.frames.size()};
      if (size == 0) throw exception("Interface '{}' contains no frames", element->name.string());
      if (current >= size) current = size - 1;
      const auto &coordinates{element->active.texture.animation.frames[current].coordinates};
      const auto &flip{element->active.texture.flip};
      const auto color{
        glm::vec4{element->active.texture.color.tint.interpolated(element->previous.texture.color.tint, alpha)}};
      const auto transparency{
        element->active.texture.color.alpha.interpolated(element->previous.texture.color.alpha, alpha)};
      const glm::mat4 model{element->active.calculate_model_matrix(element->previous,
                                                                   element->active.texture.image.frame_width,
                                                                   element->active.texture.image.frame_height, alpha)};
      graphics_object::sample data{};
      SDL_memcpy(data.model.data(), &model, sizeof(model));
      data.red = color.r;
      data.green = color.g;
      data.blue = color.b;
      data.alpha = color.a;
      data.left = static_cast<float>(flip.horizontal ? coordinates.right : coordinates.left);
      data.right = static_cast<float>(flip.horizontal ? coordinates.left : coordinates.right);
      data.top = static_cast<float>(flip.vertical ? coordinates.bottom : coordinates.top);
      data.bottom = static_cast<float>(flip.vertical ? coordinates.top : coordinates.bottom);
      data.transparency = static_cast<float>(transparency);
      auto &available{require_pipelines()};
      auto *pipe{available.interface};
      auto *texture{require_texture(element->active.texture.image)};
      if (!graphics_object.batches.empty() && graphics_object.batches.back().pipeline == pipe &&
          graphics_object.batches.back().texture == texture)
        graphics_object.batches.back().count++;
      else
        graphics_object.batches.push_back({graphics_object.samples.size(), 1, pipe, texture});
      graphics_object.samples.push_back(data);

      const auto &text{element->active.text};
      if (text.content.empty()) continue;
      if (!text.font.image.data.data()) throw exception("Interface '{}' has text but no font", element->name.string());
      if (text.font.glyphs.empty())
        throw exception("Font for interface '{}' contains no glyphs", element->name.string());
      auto &text_frame{element->active.text.playback.frame};
      const auto text_frames{text.animation.frames.size()};
      if (text_frames == 0) throw exception("Interface '{}' text contains no frames", element->name.string());
      if (text_frame >= text_frames) text_frame = text_frames - 1;
      const auto &text_coordinates{text.animation.frames[text_frame].coordinates};

      const auto text_color{glm::vec4{text.color.tint.interpolated(element->previous.text.color.tint, alpha)}};
      const auto text_alpha{text.color.alpha.interpolated(element->previous.text.color.alpha, alpha)};
      if (static_cast<int>(std::clamp(text_alpha, 0.0, 1.0) * 255.0) <= 0) continue;
      const auto text_scale{text.scale.interpolated(element->previous.text.scale, alpha)};
      const auto scale_x{std::max(1.0, std::floor(text_scale.x + 0.5))};
      const auto scale_y{std::max(1.0, std::floor(text_scale.y + 0.5))};
      const auto &scale{element->active.scale.value};
      const auto element_width{static_cast<double>(element->active.texture.image.frame_width) *
                               std::max(1.0, std::floor(scale.x + 0.5))};
      const auto element_height{static_cast<double>(element->active.texture.image.frame_height) *
                                std::max(1.0, std::floor(scale.y + 0.5))};
      const auto box_left{-element_width / 2.0};
      const auto box_right{element_width / 2.0};
      const auto box_top{element_height / 2.0};
      const auto box_bottom{-element_height / 2.0};

      constexpr std::uint32_t undefined{0xFFFD};
      const auto find{
        [&](const std::uint32_t character) -> const cse::font::glyph &
        {
          const auto locate{[&](const std::uint32_t value) -> const cse::font::glyph *
                            {
                              const auto position{std::lower_bound(
                                text.font.glyphs.begin(), text.font.glyphs.end(), static_cast<std::uint64_t>(value),
                                [](const cse::font::glyph &candidate, const std::uint64_t target)
                                { return candidate.character < target; })};
                              if (position == text.font.glyphs.end() || position->character != value) return nullptr;
                              return &*position;
                            }};
          if (const auto *glyph{locate(character)}) return *glyph;
          if (const auto *glyph{locate(undefined)}) return *glyph;
          throw exception("Font for interface '{}' is missing glyph U+{:04X} and the U+FFFD fallback glyph",
                          element->name.string(), character);
        }};
      std::vector<std::uint32_t> characters{};
      characters.reserve(text.content.size());
      for (std::size_t index{}; index < text.content.size();)
      {
        const auto first{static_cast<unsigned char>(text.content[index])};
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
        if (index + length > text.content.size())
        {
          characters.push_back(undefined);
          break;
        }
        bool malformed{false};
        for (std::size_t offset{1}; offset < length; ++offset)
        {
          const auto continuation{static_cast<unsigned char>(text.content[index + offset])};
          if ((continuation & 0xC0u) != 0x80u)
          {
            malformed = true;
            break;
          }
          character = (character << 6) | (continuation & 0x3Fu);
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

      const auto spacing_x{
        text.align.horizontal.spacing.interpolated(element->previous.text.align.horizontal.spacing, alpha)};
      const auto spacing_y{
        text.align.vertical.spacing.interpolated(element->previous.text.align.vertical.spacing, alpha)};
      const auto line_height{text.font.glyphs.front().height * scale_y};
      struct item
      {
        const cse::font::glyph *glyph{};
        std::uint32_t character{};
      };
      struct line
      {
        std::vector<item> items{};
        double width{};
      };
      std::vector<line> lines{};
      lines.emplace_back();
      const auto append{[&](const cse::font::glyph &glyph, const std::uint32_t character)
                        {
                          auto &active_line{lines.back()};
                          active_line.width += (active_line.items.empty() ? 0.0 : spacing_x) + glyph.width * scale_x;
                          active_line.items.push_back({&glyph, character});
                        }};
      const auto strip{[&]()
                       {
                         auto &active_line{lines.back()};
                         while (!active_line.items.empty() && active_line.items.back().character == U' ')
                         {
                           active_line.width -= active_line.items.back().glyph->width * scale_x;
                           active_line.items.pop_back();
                           if (!active_line.items.empty()) active_line.width -= spacing_x;
                         }
                       }};
      const auto commit{[&]()
                        {
                          strip();
                          lines.emplace_back();
                        }};
      std::vector<item> word{};
      double word_width{};
      const auto flush{[&]()
                       {
                         if (word.empty()) return;
                         if (text.overflow.wrap && !lines.back().items.empty() &&
                             lines.back().width + spacing_x + word_width > element_width)
                           commit();
                         for (const auto &entry : word)
                         {
                           if (text.overflow.wrap && !lines.back().items.empty() &&
                               lines.back().width + spacing_x + entry.glyph->width * scale_x > element_width)
                             commit();
                           append(*entry.glyph, entry.character);
                         }
                         word.clear();
                         word_width = 0.0;
                       }};
      for (const auto character : characters)
      {
        if (character == U'\n')
        {
          flush();
          commit();
          continue;
        }
        const auto &glyph{find(character)};
        if (character == U' ')
        {
          flush();
          append(glyph, character);
          continue;
        }
        word_width += (word.empty() ? 0.0 : spacing_x) + glyph.width * scale_x;
        word.push_back({&glyph, character});
      }
      flush();
      strip();

      double block_width{};
      for (const auto &entry : lines) block_width = std::max(block_width, entry.width);
      const auto line_count{static_cast<double>(lines.size())};
      const auto block_height{line_count * line_height + (line_count - 1.0) * spacing_y};
      const glm::dvec2 shift{text.align.offset.interpolated(element->previous.text.align.offset, alpha)};
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

      auto *atlas{require_texture(text.font.image)};
      for (std::size_t row{}; row < lines.size(); ++row)
      {
        const auto &entry{lines[row]};
        double pen{block_left};
        if (text.align.horizontal.preset == CENTER)
          pen = block_left + (block_width - entry.width) / 2.0;
        else if (text.align.horizontal.preset == RIGHT)
          pen = block_left + (block_width - entry.width);
        const auto top{block_top - static_cast<double>(row) * (line_height + spacing_y)};
        for (const auto &placed : entry.items)
        {
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
                             (text_coordinates.right - text_coordinates.left) * glyph_coordinates.left};
          const auto uv_right{text_coordinates.left +
                              (text_coordinates.right - text_coordinates.left) * glyph_coordinates.right};
          const auto uv_top{text_coordinates.bottom +
                            (text_coordinates.top - text_coordinates.bottom) * glyph_coordinates.top};
          const auto uv_bottom{text_coordinates.bottom +
                               (text_coordinates.top - text_coordinates.bottom) * glyph_coordinates.bottom};
          const auto fraction_left{(visible_left - left) / width};
          const auto fraction_right{(visible_right - left) / width};
          const auto fraction_top{(top - visible_top) / height};
          const auto fraction_bottom{(top - visible_bottom) / height};
          const glm::mat4 text_model{element->active.calculate_text_matrix(
            element->previous, visible_right - visible_left, visible_top - visible_bottom,
            {(visible_left + visible_right) / 2.0, (visible_top + visible_bottom) / 2.0}, alpha)};
          graphics_object::sample text_data{};
          SDL_memcpy(text_data.model.data(), &text_model, sizeof(text_model));
          text_data.red = text_color.r;
          text_data.green = text_color.g;
          text_data.blue = text_color.b;
          text_data.alpha = text_color.a;
          text_data.left = static_cast<float>(uv_left + (uv_right - uv_left) * fraction_left);
          text_data.right = static_cast<float>(uv_left + (uv_right - uv_left) * fraction_right);
          text_data.top = static_cast<float>(uv_top + (uv_bottom - uv_top) * fraction_top);
          text_data.bottom = static_cast<float>(uv_top + (uv_bottom - uv_top) * fraction_bottom);
          text_data.transparency = static_cast<float>(text_alpha);
          if (!graphics_object.batches.empty() && graphics_object.batches.back().pipeline == pipe &&
              graphics_object.batches.back().texture == atlas)
            graphics_object.batches.back().count++;
          else
            graphics_object.batches.push_back({graphics_object.samples.size(), 1, pipe, atlas});
          graphics_object.samples.push_back(text_data);
        }
      }
    }
    upload_samples();
  }

  void active::generate_objects(const std::vector<cse::object *> &object_order)
  {
    graphics_object.samples.clear();
    graphics_object.batches.clear();
    graphics_object.samples.reserve(object_order.size());
    static constexpr double depth_bias_span{0.001};
    const auto object_count{static_cast<double>(object_order.size())};
    const double depth_bias_step{object_order.empty() ? 0.0 : depth_bias_span / object_count};
    for (std::size_t position{}; position < object_order.size(); ++position)
    {
      auto *element{object_order[position]};
      auto &current{element->active.texture.playback.frame};
      const auto size{element->active.texture.animation.frames.size()};
      if (size == 0) throw exception("Object '{}' contains no frames", element->name.string());
      if (current >= size) current = size - 1;
      const auto &coordinates{element->active.texture.animation.frames[current].coordinates};
      const auto &flip{element->active.texture.flip};
      const auto color{
        glm::vec4{element->active.texture.color.tint.interpolated(element->previous.texture.color.tint, alpha)}};
      const auto transparency{
        element->active.texture.color.alpha.interpolated(element->previous.texture.color.alpha, alpha)};
      const glm::mat4 model{element->active.calculate_model_matrix(element->previous,
                                                                   element->active.texture.image.frame_width,
                                                                   element->active.texture.image.frame_height, alpha)};
      graphics_object::sample data{};
      SDL_memcpy(data.model.data(), &model, sizeof(model));
      data.red = color.r;
      data.green = color.g;
      data.blue = color.b;
      data.alpha = color.a;
      data.left = static_cast<float>(flip.horizontal ? coordinates.right : coordinates.left);
      data.bottom = static_cast<float>(flip.vertical ? coordinates.top : coordinates.bottom);
      data.right = static_cast<float>(flip.horizontal ? coordinates.left : coordinates.right);
      data.top = static_cast<float>(flip.vertical ? coordinates.bottom : coordinates.top);
      data.lit = element->active.illumination.show ? 1.0f : 0.0f;
      data.shadowed = element->active.shadow.show ? 1.0f : 0.0f;
      data.brightness = static_cast<float>(
        element->active.illumination.brightness.interpolated(element->previous.illumination.brightness, alpha));
      data.transparency = static_cast<float>(transparency);
      data.depth = static_cast<float>(static_cast<double>(graphics_object.samples.size()) * depth_bias_step);
      data.occluder = position < graphics_occluder.indices.size() ? graphics_occluder.indices[position] : -1.0f;
      auto &available{require_pipelines()};
      auto *pipe{transparency < 1.0 ? available.transparent : available.opaque};
      auto *texture{require_texture(element->active.texture.image)};
      if (!graphics_object.batches.empty() && graphics_object.batches.back().pipeline == pipe &&
          graphics_object.batches.back().texture == texture)
        graphics_object.batches.back().count++;
      else
        graphics_object.batches.push_back({graphics_object.samples.size(), 1, pipe, texture});
      graphics_object.samples.push_back(data);
    }
    upload_samples();
  }

  void active::generate_lights(const std::vector<cse::light *> &light_order)
  {
    graphics_light.data.meta[0] = static_cast<float>(light_order.size());
    graphics_light.samples.clear();
    graphics_light.samples.reserve(light_order.size());
    for (auto *element : light_order)
    {
      const auto interpolated{element->active.translation.interpolated(element->previous.translation, alpha)};
      const glm::dvec3 position{std::floor(interpolated.x + 0.5), std::floor(interpolated.y + 0.5),
                                std::floor(interpolated.z + 0.5)};
      const auto direction{element->active.calculate_direction(element->previous, alpha)};
      const auto brightness{
        element->active.illumination.brightness.interpolated(element->previous.illumination.brightness, alpha)};
      const auto penetration{std::max(
        0.0, element->active.illumination.penetration.interpolated(element->previous.illumination.penetration, alpha))};
      const auto range{element->active.illumination.range.interpolated(element->previous.illumination.range, alpha)};
      const auto angle{element->active.illumination.angle.interpolated(element->previous.illumination.angle, alpha)};
      const auto softness{std::clamp(
        element->active.illumination.softness.interpolated(element->previous.illumination.softness, alpha), 0.0, 1.0)};
      const auto &shadow{element->active.shadow};
      const auto shadow_darkness{shadow.darkness.interpolated(element->previous.shadow.darkness, alpha)};
      const auto shadow_softness{shadow.softness.interpolated(element->previous.shadow.softness, alpha)};
      graphics_light::entry entry{};
      entry.position[0] = static_cast<float>(position.x);
      entry.position[1] = static_cast<float>(position.y);
      entry.position[2] = static_cast<float>(position.z);
      entry.position[3] = static_cast<float>(range);
      entry.brightness[0] = static_cast<float>(brightness.x * brightness.w);
      entry.brightness[1] = static_cast<float>(brightness.y * brightness.w);
      entry.brightness[2] = static_cast<float>(brightness.z * brightness.w);
      entry.brightness[3] = shadow.cast ? static_cast<float>(std::max(0.0, shadow_darkness)) : 0.0f;
      entry.direction[0] = static_cast<float>(direction.x);
      entry.direction[1] = static_cast<float>(direction.y);
      entry.direction[2] = static_cast<float>(direction.z);
      entry.direction[3] = element->active.illumination.global ? 1.0f : 0.0f;
      const auto half{angle / 2.0};
      entry.cone[0] = static_cast<float>(std::cos(glm::radians(half)));
      entry.cone[1] = static_cast<float>(std::cos(glm::radians(half * (1.0 - softness))));
      entry.cone[2] = static_cast<float>(penetration);
      entry.cone[3] = static_cast<float>(std::clamp(shadow_softness, 0.0, 1.0));
      graphics_light.samples.push_back(entry);
    }

    if (graphics_light.samples.empty()) return;
    if (graphics_light.samples.size() > graphics_light.capacity)
    {
      SDL_ReleaseGPUTransferBuffer(device, graphics_light.transfer_buffer);
      SDL_ReleaseGPUBuffer(device, graphics_light.buffer);
      graphics_light.capacity = std::max<std::size_t>(graphics_light.capacity, 16);
      while (graphics_light.capacity < graphics_light.samples.size()) graphics_light.capacity *= 2;
      SDL_GPUBufferCreateInfo buffer_info{
        .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
        .size = static_cast<Uint32>(sizeof(graphics_light::entry) * graphics_light.capacity),
        .props = 0};
      graphics_light.buffer = SDL_CreateGPUBuffer(device, &buffer_info);
      if (!graphics_light.buffer) throw sdl_exception("Could not create light storage buffer for game");
      SDL_GPUTransferBufferCreateInfo transfer_buffer_info{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = static_cast<Uint32>(sizeof(graphics_light::entry) * graphics_light.capacity),
        .props = 0};
      graphics_light.transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_info);
      if (!graphics_light.transfer_buffer) throw sdl_exception("Could not create light transfer buffer for game");
    }
    auto *start{SDL_MapGPUTransferBuffer(device, graphics_light.transfer_buffer, true)};
    if (!start) throw sdl_exception("Could not map light data for game");
    SDL_memcpy(start, graphics_light.samples.data(), sizeof(graphics_light::entry) * graphics_light.samples.size());
    SDL_UnmapGPUTransferBuffer(device, graphics_light.transfer_buffer);
    auto *command_buffer{SDL_AcquireGPUCommandBuffer(device)};
    if (!command_buffer) throw sdl_exception("Could not acquire GPU command buffer for game");
    auto *copy_pass{SDL_BeginGPUCopyPass(command_buffer)};
    if (!copy_pass) throw sdl_exception("Could not begin GPU copy pass for game");
    SDL_GPUTransferBufferLocation transfer_location{.transfer_buffer = graphics_light.transfer_buffer, .offset = 0};
    SDL_GPUBufferRegion buffer_region{
      .buffer = graphics_light.buffer,
      .offset = 0,
      .size = static_cast<Uint32>(sizeof(graphics_light::entry) * graphics_light.samples.size())};
    SDL_UploadToGPUBuffer(copy_pass, &transfer_location, &buffer_region, true);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
  }

  void active::generate_occluders(const std::vector<cse::object *> &object_order)
  {
    graphics_occluder.samples.clear();

    static std::vector<const cse::image *> images{};
    static std::vector<const unsigned char *> signature{};
    images.clear();
    signature.clear();
    const auto layer_of{[&](const cse::image &image) -> int
                        {
                          for (std::size_t index{}; index < signature.size(); ++index)
                            if (signature[index] == image.data.data()) return static_cast<int>(index);
                          signature.push_back(image.data.data());
                          images.push_back(&image);
                          return static_cast<int>(signature.size() - 1);
                        }};

    bool penetrating{false};
    for (const auto &sample : graphics_light.samples)
      if (sample.direction[3] < 0.5f && std::abs(sample.cone[2] - 1.0f) > 1e-3f)
      {
        penetrating = true;
        break;
      }

    graphics_occluder.indices.assign(object_order.size(), -1.0f);
    for (std::size_t position{}; position < object_order.size(); ++position)
    {
      auto *element{object_order[position]};
      const auto penetration{std::max(
        0.0, element->active.illumination.penetration.interpolated(element->previous.illumination.penetration, alpha))};
      if (!element->active.shadow.cast && !penetrating && std::abs(penetration - 1.0) < 1e-6) continue;
      const auto &image{element->active.texture.image};
      const auto frame_count{element->active.texture.animation.frames.size()};
      if (!image.data.data() || frame_count == 0) continue;
      auto frame_index{element->active.texture.playback.frame};
      if (frame_index >= frame_count) frame_index = frame_count - 1;
      const auto &coordinates{element->active.texture.animation.frames[frame_index].coordinates};
      const auto &flip{element->active.texture.flip};
      const auto translation{element->active.translation.interpolated(element->previous.translation, alpha)};
      const auto rotation{element->active.rotation.interpolated(element->previous.rotation, alpha)};
      const auto scale{element->active.scale.interpolated(element->previous.scale, alpha)};
      const auto transparency{std::clamp(
        element->active.texture.color.alpha.interpolated(element->previous.texture.color.alpha, alpha), 0.0, 1.0)};
      const int steps{((static_cast<int>(std::floor(rotation + 0.5)) % 4) + 4) % 4};
      const bool rotated{steps % 2 == 1};
      const double width{static_cast<double>(image.frame_width)};
      const double height{static_cast<double>(image.frame_height)};
      const double cx{std::floor(translation.x + 0.5) - (image.frame_width % 2 == 1 ? 0.5 : 0.0)};
      const double cy{std::floor(translation.y + 0.5) - (image.frame_height % 2 == 1 ? 0.5 : 0.0)};
      const double cz{std::floor(translation.z + 0.5)};
      const double hx{rotated ? std::floor(scale.y + 0.5) * height / 2.0 : std::floor(scale.x + 0.5) * width / 2.0};
      const double hy{rotated ? std::floor(scale.x + 0.5) * width / 2.0 : std::floor(scale.y + 0.5) * height / 2.0};
      graphics_occluder::entry entry{};
      entry.rectangle[0] = static_cast<float>(cx - hx);
      entry.rectangle[1] = static_cast<float>(cy - hy);
      entry.rectangle[2] = static_cast<float>(cx + hx);
      entry.rectangle[3] = static_cast<float>(cy + hy);
      const auto first_u{flip.horizontal ? coordinates.right : coordinates.left};
      const auto second_u{flip.horizontal ? coordinates.left : coordinates.right};
      const auto first_v{flip.vertical ? coordinates.top : coordinates.bottom};
      const auto second_v{flip.vertical ? coordinates.bottom : coordinates.top};
      const bool swap_u{steps == 1 || steps == 2};
      const bool swap_v{steps == 2 || steps == 3};
      entry.frame[0] = static_cast<float>(swap_u ? second_u : first_u);
      entry.frame[1] = static_cast<float>(swap_v ? second_v : first_v);
      entry.frame[2] = static_cast<float>(swap_u ? first_u : second_u);
      entry.frame[3] = static_cast<float>(swap_v ? first_v : second_v);
      const auto shadow_darkness{
        element->active.shadow.darkness.interpolated(element->previous.shadow.darkness, alpha)};
      const auto shadow_softness{
        element->active.shadow.softness.interpolated(element->previous.shadow.softness, alpha)};
      entry.surface[0] = static_cast<float>(cz);
      entry.surface[1] = static_cast<float>(layer_of(image));
      entry.surface[2] = static_cast<float>(transparency);
      entry.surface[3] = rotated ? 1.0f : 0.0f;
      entry.shadow[0] = static_cast<float>(penetration);
      entry.shadow[1] = element->active.shadow.cast ? 1.0f : 0.0f;
      entry.shadow[2] = static_cast<float>(std::max(0.0, shadow_darkness));
      entry.shadow[3] = static_cast<float>(std::clamp(shadow_softness, 0.0, 1.0));
      graphics_occluder.indices[position] = static_cast<float>(graphics_occluder.samples.size());
      graphics_occluder.samples.push_back(entry);
    }
    graphics_light.data.meta[1] = static_cast<float>(graphics_occluder.samples.size());

    if (!images.empty())
    {
      unsigned int max_width{1};
      unsigned int max_height{1};
      for (const auto *image : images)
      {
        max_width = std::max(max_width, image->width);
        max_height = std::max(max_height, image->height);
      }
      for (auto &entry : graphics_occluder.samples)
      {
        const auto *image{images[static_cast<std::size_t>(entry.surface[1])]};
        const float scale_u{static_cast<float>(image->width) / static_cast<float>(max_width)};
        const float scale_v{static_cast<float>(image->height) / static_cast<float>(max_height)};
        entry.frame[0] *= scale_u;
        entry.frame[2] *= scale_u;
        entry.frame[1] *= scale_v;
        entry.frame[3] *= scale_v;
      }
      if (signature != graphics_occluder.signature || max_width != graphics_occluder.width ||
          max_height != graphics_occluder.height)
      {
        SDL_ReleaseGPUTexture(device, graphics_occluder.texture);
        SDL_GPUTextureCreateInfo array_info{.type = SDL_GPU_TEXTURETYPE_2D_ARRAY,
                                            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                            .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
                                            .width = max_width,
                                            .height = max_height,
                                            .layer_count_or_depth = static_cast<Uint32>(images.size()),
                                            .num_levels = 1,
                                            .sample_count = SDL_GPU_SAMPLECOUNT_1,
                                            .props = 0};
        graphics_occluder.texture = SDL_CreateGPUTexture(device, &array_info);
        if (!graphics_occluder.texture) throw sdl_exception("Could not create occluder texture array for game");
        for (std::size_t layer{}; layer < images.size(); ++layer)
        {
          const auto *image{images[layer]};
          SDL_GPUTransferBufferCreateInfo transfer_info{.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                                        .size = image->width * image->height * image->channels,
                                                        .props = 0};
          auto *transfer{SDL_CreateGPUTransferBuffer(device, &transfer_info)};
          if (!transfer) throw sdl_exception("Could not create occluder layer transfer buffer for game");
          auto *pixels{SDL_MapGPUTransferBuffer(device, transfer, false)};
          if (!pixels) throw sdl_exception("Could not map occluder layer data for game");
          SDL_memcpy(pixels, image->data.data(), image->width * image->height * image->channels);
          SDL_UnmapGPUTransferBuffer(device, transfer);
          auto *command_buffer{SDL_AcquireGPUCommandBuffer(device)};
          if (!command_buffer) throw sdl_exception("Could not acquire GPU command buffer for game");
          auto *copy_pass{SDL_BeginGPUCopyPass(command_buffer)};
          if (!copy_pass) throw sdl_exception("Could not begin GPU copy pass for game");
          SDL_GPUTextureTransferInfo source{
            .transfer_buffer = transfer, .offset = 0, .pixels_per_row = 0, .rows_per_layer = 0};
          SDL_GPUTextureRegion region{.texture = graphics_occluder.texture,
                                      .mip_level = 0,
                                      .layer = static_cast<Uint32>(layer),
                                      .x = 0,
                                      .y = 0,
                                      .z = 0,
                                      .w = image->width,
                                      .h = image->height,
                                      .d = 1};
          SDL_UploadToGPUTexture(copy_pass, &source, &region, false);
          SDL_EndGPUCopyPass(copy_pass);
          SDL_SubmitGPUCommandBuffer(command_buffer);
          SDL_ReleaseGPUTransferBuffer(device, transfer);
        }
        graphics_occluder.width = max_width;
        graphics_occluder.height = max_height;
        graphics_occluder.layers = static_cast<unsigned int>(images.size());
      }
    }
    graphics_occluder.signature = signature;
    graphics_light.data.meta[2] = static_cast<float>(graphics_occluder.width);
    graphics_light.data.meta[3] = static_cast<float>(graphics_occluder.height);

    if (graphics_occluder.samples.empty()) return;
    if (graphics_occluder.samples.size() > graphics_occluder.capacity)
    {
      SDL_ReleaseGPUTransferBuffer(device, graphics_occluder.transfer_buffer);
      SDL_ReleaseGPUBuffer(device, graphics_occluder.buffer);
      graphics_occluder.capacity = std::max<std::size_t>(graphics_occluder.capacity, 16);
      while (graphics_occluder.capacity < graphics_occluder.samples.size()) graphics_occluder.capacity *= 2;
      SDL_GPUBufferCreateInfo buffer_info{
        .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
        .size = static_cast<Uint32>(sizeof(graphics_occluder::entry) * graphics_occluder.capacity),
        .props = 0};
      graphics_occluder.buffer = SDL_CreateGPUBuffer(device, &buffer_info);
      if (!graphics_occluder.buffer) throw sdl_exception("Could not create occluder storage buffer for game");
      SDL_GPUTransferBufferCreateInfo transfer_buffer_info{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = static_cast<Uint32>(sizeof(graphics_occluder::entry) * graphics_occluder.capacity),
        .props = 0};
      graphics_occluder.transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_info);
      if (!graphics_occluder.transfer_buffer) throw sdl_exception("Could not create occluder transfer buffer for game");
    }
    auto *start{SDL_MapGPUTransferBuffer(device, graphics_occluder.transfer_buffer, true)};
    if (!start) throw sdl_exception("Could not map occluder data for game");
    SDL_memcpy(start, graphics_occluder.samples.data(),
               sizeof(graphics_occluder::entry) * graphics_occluder.samples.size());
    SDL_UnmapGPUTransferBuffer(device, graphics_occluder.transfer_buffer);
    auto *command_buffer{SDL_AcquireGPUCommandBuffer(device)};
    if (!command_buffer) throw sdl_exception("Could not acquire GPU command buffer for game");
    auto *copy_pass{SDL_BeginGPUCopyPass(command_buffer)};
    if (!copy_pass) throw sdl_exception("Could not begin GPU copy pass for game");
    SDL_GPUTransferBufferLocation transfer_location{.transfer_buffer = graphics_occluder.transfer_buffer, .offset = 0};
    SDL_GPUBufferRegion buffer_region{
      .buffer = graphics_occluder.buffer,
      .offset = 0,
      .size = static_cast<Uint32>(sizeof(graphics_occluder::entry) * graphics_occluder.samples.size())};
    SDL_UploadToGPUBuffer(copy_pass, &transfer_location, &buffer_region, true);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
  }

  void active::upload_samples()
  {
    if (graphics_object.samples.empty()) return;
    if (graphics_object.samples.size() > graphics_object.capacity)
    {
      SDL_ReleaseGPUTransferBuffer(device, graphics_object.transfer_buffer);
      SDL_ReleaseGPUBuffer(device, graphics_object.buffer);
      graphics_object.capacity = std::max<std::size_t>(graphics_object.capacity, 16);
      while (graphics_object.capacity < graphics_object.samples.size()) graphics_object.capacity *= 2;
      SDL_GPUBufferCreateInfo instance_buffer_info{
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = static_cast<Uint32>(sizeof(graphics_object::sample) * graphics_object.capacity),
        .props = 0};
      graphics_object.buffer = SDL_CreateGPUBuffer(device, &instance_buffer_info);
      if (!graphics_object.buffer) throw sdl_exception("Could not create instance buffer for game");
      SDL_GPUTransferBufferCreateInfo instance_transfer_buffer_info{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = static_cast<Uint32>(sizeof(graphics_object::sample) * graphics_object.capacity),
        .props = 0};
      graphics_object.transfer_buffer = SDL_CreateGPUTransferBuffer(device, &instance_transfer_buffer_info);
      if (!graphics_object.transfer_buffer) throw sdl_exception("Could not create instance transfer buffer for game");
    }
    auto *start{SDL_MapGPUTransferBuffer(device, graphics_object.transfer_buffer, true)};
    if (!start) throw sdl_exception("Could not map instance data for game");
    SDL_memcpy(start, graphics_object.samples.data(), sizeof(graphics_object::sample) * graphics_object.samples.size());
    SDL_UnmapGPUTransferBuffer(device, graphics_object.transfer_buffer);
    auto *command_buffer{SDL_AcquireGPUCommandBuffer(device)};
    if (!command_buffer) throw sdl_exception("Could not acquire GPU command buffer for game");
    auto *copy_pass{SDL_BeginGPUCopyPass(command_buffer)};
    if (!copy_pass) throw sdl_exception("Could not begin GPU copy pass for game");
    SDL_GPUTransferBufferLocation instance_transfer_location{.transfer_buffer = graphics_object.transfer_buffer,
                                                             .offset = 0};
    SDL_GPUBufferRegion instance_buffer_region{
      .buffer = graphics_object.buffer,
      .offset = 0,
      .size = static_cast<Uint32>(sizeof(graphics_object::sample) * graphics_object.samples.size())};
    SDL_UploadToGPUBuffer(copy_pass, &instance_transfer_location, &instance_buffer_region, true);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
  }

  void active::draw_batches(const std::pair<glm::dmat4, glm::dmat4> &matrices)
  {
    if (graphics_object.batches.empty()) return;
    const auto &[projection_matrix, view_matrix] = matrices;
    const std::array<glm::mat4, 2> data{glm::mat4{projection_matrix}, glm::mat4{view_matrix}};
    SDL_PushGPUVertexUniformData(window->active.command_buffer, 0, &data, sizeof(data));
    SDL_PushGPUFragmentUniformData(window->active.command_buffer, 0, &graphics_light.data, sizeof(graphics_light.data));
    const std::array<SDL_GPUBufferBinding, 2> vertex_buffer_bindings{
      {{.buffer = graphics_buffer.vertex, .offset = 0}, {.buffer = graphics_object.buffer, .offset = 0}}};
    SDL_BindGPUVertexBuffers(window->active.render_pass, 0, vertex_buffer_bindings.data(), 2);
    SDL_GPUBufferBinding index_buffer_binding{.buffer = graphics_buffer.index, .offset = 0};
    SDL_BindGPUIndexBuffer(window->active.render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    const std::array<SDL_GPUBuffer *, 2> storage_buffers{graphics_light.buffer, graphics_occluder.buffer};
    SDL_BindGPUFragmentStorageBuffers(window->active.render_pass, 0, storage_buffers.data(), 2);
    for (const auto &group : graphics_object.batches)
    {
      SDL_BindGPUGraphicsPipeline(window->active.render_pass, group.pipeline);
      const std::array<SDL_GPUTextureSamplerBinding, 2> texture_sampler_bindings{
        {{.texture = group.texture, .sampler = graphics_buffer.nearest},
         {.texture = graphics_occluder.texture, .sampler = graphics_buffer.linear}}};
      SDL_BindGPUFragmentSamplers(window->active.render_pass, 0, texture_sampler_bindings.data(), 2);
      SDL_DrawGPUIndexedPrimitives(window->active.render_pass, 6, static_cast<Uint32>(group.count), 0, 0,
                                   static_cast<Uint32>(group.first));
    }
  }

  struct active::graphics_pipeline &active::require_pipelines()
  {
    if (graphics_pipeline.opaque) return graphics_pipeline;
    const auto backend_formats{SDL_GetGPUShaderFormats(device)};
    if (!has(backend_formats, SDL_GPU_SHADERFORMAT_SPIRV))
      throw sdl_exception("No supported vulkan shader formats for game");
    SDL_GPUShaderCreateInfo vertex_shader_info{.code_size = shader::vertex.size(),
                                               .code = shader::vertex.data(),
                                               .entrypoint = "main",
                                               .format = SDL_GPU_SHADERFORMAT_SPIRV,
                                               .stage = SDL_GPU_SHADERSTAGE_VERTEX,
                                               .num_samplers = 0,
                                               .num_storage_textures = 0,
                                               .num_storage_buffers = 0,
                                               .num_uniform_buffers = 1,
                                               .props = 0};
    auto *vertex_shader{SDL_CreateGPUShader(device, &vertex_shader_info)};
    if (!vertex_shader) throw sdl_exception("Could not create vertex shader for game");
    SDL_GPUShaderCreateInfo fragment_shader_info{.code_size = shader::fragment.size(),
                                                 .code = shader::fragment.data(),
                                                 .entrypoint = "main",
                                                 .format = SDL_GPU_SHADERFORMAT_SPIRV,
                                                 .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
                                                 .num_samplers = 2,
                                                 .num_storage_textures = 0,
                                                 .num_storage_buffers = 2,
                                                 .num_uniform_buffers = 1,
                                                 .props = 0};
    auto *fragment_shader{SDL_CreateGPUShader(device, &fragment_shader_info)};
    if (!fragment_shader) throw sdl_exception("Could not create fragment shader for game");
    const std::array<SDL_GPUVertexBufferDescription, 2> vertex_buffer_descriptions{
      {{.slot = 0, .pitch = sizeof(corner), .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX, .instance_step_rate = 0},
       {.slot = 1,
        .pitch = sizeof(graphics_object::sample),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
        .instance_step_rate = 0}}};
    const std::array<SDL_GPUVertexAttribute, 9> vertex_attributes{
      {{0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(corner, x)},
       {1, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(graphics_object::sample, model)},
       {2, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(graphics_object::sample, model) + sizeof(float) * 4},
       {3, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(graphics_object::sample, model) + sizeof(float) * 8},
       {4, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(graphics_object::sample, model) + sizeof(float) * 12},
       {5, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(graphics_object::sample, red)},
       {6, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(graphics_object::sample, left)},
       {7, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(graphics_object::sample, lit)},
       {8, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(graphics_object::sample, depth)}}};
    SDL_GPUVertexInputState vertex_input_state{.vertex_buffer_descriptions = vertex_buffer_descriptions.data(),
                                               .num_vertex_buffers = 2,
                                               .vertex_attributes = vertex_attributes.data(),
                                               .num_vertex_attributes = 9};
    SDL_GPURasterizerState rasterizer_state{};
    rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    rasterizer_state.enable_depth_clip = true;
    SDL_GPUColorTargetDescription opaque_color_target_description{};
    opaque_color_target_description.format = SDL_GetGPUSwapchainTextureFormat(device, window->active.instance);
    SDL_GPUDepthStencilState opaque_depth_stencil_state{};
    opaque_depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    opaque_depth_stencil_state.enable_depth_test = true;
    opaque_depth_stencil_state.enable_depth_write = true;
    const auto type{SDL_GPU_TEXTURETYPE_2D};
    const std::array<SDL_GPUTextureFormat, 3> potential_formats{
      SDL_GPU_TEXTUREFORMAT_D32_FLOAT, SDL_GPU_TEXTUREFORMAT_D24_UNORM, SDL_GPU_TEXTUREFORMAT_D16_UNORM};
    SDL_GPUGraphicsPipelineTargetInfo opaque_target_info{};
    opaque_target_info.color_target_descriptions = &opaque_color_target_description;
    opaque_target_info.num_color_targets = 1;
    opaque_target_info.depth_stencil_format = [this, &potential_formats]() -> SDL_GPUTextureFormat
    {
      for (const auto &potential_format : potential_formats)
        if (SDL_GPUTextureSupportsFormat(device, potential_format, type, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
          return potential_format;
      return {};
    }();
    opaque_target_info.has_depth_stencil_target = true;
    if (opaque_target_info.depth_stencil_format == SDL_GPU_TEXTUREFORMAT_INVALID)
      throw sdl_exception("No supported depth stencil format found for game");
    SDL_GPUGraphicsPipelineCreateInfo opaque_pipeline_info{};
    opaque_pipeline_info.vertex_shader = vertex_shader;
    opaque_pipeline_info.fragment_shader = fragment_shader;
    opaque_pipeline_info.vertex_input_state = vertex_input_state;
    opaque_pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    opaque_pipeline_info.rasterizer_state = rasterizer_state;
    opaque_pipeline_info.depth_stencil_state = opaque_depth_stencil_state;
    opaque_pipeline_info.target_info = opaque_target_info;
    graphics_pipeline.opaque = SDL_CreateGPUGraphicsPipeline(device, &opaque_pipeline_info);
    if (!graphics_pipeline.opaque) throw sdl_exception("Could not create graphics pipeline for game");
    SDL_GPUColorTargetBlendState blend_state{};
    blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    blend_state.color_write_mask = 0;
    blend_state.enable_blend = true;
    blend_state.enable_color_write_mask = false;
    SDL_GPUColorTargetDescription transparent_color_target_description{};
    transparent_color_target_description.format = SDL_GetGPUSwapchainTextureFormat(device, window->active.instance);
    transparent_color_target_description.blend_state = blend_state;
    SDL_GPUDepthStencilState transparent_depth_stencil_state{};
    transparent_depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    transparent_depth_stencil_state.enable_depth_test = true;
    transparent_depth_stencil_state.enable_depth_write = false;
    SDL_GPUGraphicsPipelineTargetInfo transparent_target_info{};
    transparent_target_info.color_target_descriptions = &transparent_color_target_description;
    transparent_target_info.num_color_targets = 1;
    transparent_target_info.depth_stencil_format = opaque_target_info.depth_stencil_format;
    transparent_target_info.has_depth_stencil_target = true;
    SDL_GPUGraphicsPipelineCreateInfo transparent_pipeline_info{};
    transparent_pipeline_info.vertex_shader = vertex_shader;
    transparent_pipeline_info.fragment_shader = fragment_shader;
    transparent_pipeline_info.vertex_input_state = vertex_input_state;
    transparent_pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    transparent_pipeline_info.rasterizer_state = rasterizer_state;
    transparent_pipeline_info.depth_stencil_state = transparent_depth_stencil_state;
    transparent_pipeline_info.target_info = transparent_target_info;
    graphics_pipeline.transparent = SDL_CreateGPUGraphicsPipeline(device, &transparent_pipeline_info);
    if (!graphics_pipeline.transparent) throw sdl_exception("Could not create transparent graphics pipeline for game");
    SDL_GPURasterizerState interface_rasterizer_state{};
    interface_rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    interface_rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    interface_rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    SDL_GPUDepthStencilState interface_depth_stencil_state{};
    interface_depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
    interface_depth_stencil_state.enable_depth_test = false;
    interface_depth_stencil_state.enable_depth_write = false;
    SDL_GPUGraphicsPipelineCreateInfo interface_pipeline_info{};
    interface_pipeline_info.vertex_shader = vertex_shader;
    interface_pipeline_info.fragment_shader = fragment_shader;
    interface_pipeline_info.vertex_input_state = vertex_input_state;
    interface_pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    interface_pipeline_info.rasterizer_state = interface_rasterizer_state;
    interface_pipeline_info.depth_stencil_state = interface_depth_stencil_state;
    interface_pipeline_info.target_info = transparent_target_info;
    graphics_pipeline.interface = SDL_CreateGPUGraphicsPipeline(device, &interface_pipeline_info);
    if (!graphics_pipeline.interface) throw sdl_exception("Could not create interface graphics pipeline for game");
    SDL_ReleaseGPUShader(device, fragment_shader);
    SDL_ReleaseGPUShader(device, vertex_shader);
    return graphics_pipeline;
  }

  SDL_GPUTexture *active::require_texture(const cse::image &image)
  {
    const graphics_cache::texture_key key{image.data.data(), image.data.size()};
    if (const auto iterator{graphics_cache.texture.find(key)}; iterator != graphics_cache.texture.end())
    {
      iterator->second.stamp = graphics_cache.stamp;
      return iterator->second.value;
    }
    csp::verify(image.data.data(), image.data.size());
    const auto type{SDL_GPU_TEXTURETYPE_2D};
    const auto format{SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM};
    const auto usage{SDL_GPU_TEXTUREUSAGE_SAMPLER};
    if (!SDL_GPUTextureSupportsFormat(device, format, type, usage))
      throw sdl_exception("No supported texture format found for game");
    SDL_GPUTextureCreateInfo texture_info{.type = type,
                                          .format = format,
                                          .usage = usage,
                                          .width = image.width,
                                          .height = image.height,
                                          .layer_count_or_depth = 1,
                                          .num_levels = 1,
                                          .sample_count = SDL_GPU_SAMPLECOUNT_1,
                                          .props = 0};
    auto *texture{SDL_CreateGPUTexture(device, &texture_info)};
    if (!texture) throw sdl_exception("Could not create texture for game");
    SDL_GPUTransferBufferCreateInfo texture_transfer_buffer_info{
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = image.width * image.height * image.channels, .props = 0};
    auto *texture_transfer_buffer{SDL_CreateGPUTransferBuffer(device, &texture_transfer_buffer_info)};
    if (!texture_transfer_buffer) throw sdl_exception("Could not create transfer buffer for texture for game");
    auto *texture_data{SDL_MapGPUTransferBuffer(device, texture_transfer_buffer, false)};
    if (!texture_data) throw sdl_exception("Could not map texture data for game");
    SDL_memcpy(texture_data, image.data.data(), image.width * image.height * image.channels);
    SDL_UnmapGPUTransferBuffer(device, texture_transfer_buffer);
    auto *command_buffer{SDL_AcquireGPUCommandBuffer(device)};
    if (!command_buffer) throw sdl_exception("Could not acquire GPU command buffer for game");
    auto *copy_pass{SDL_BeginGPUCopyPass(command_buffer)};
    if (!copy_pass) throw sdl_exception("Could not begin GPU copy pass for game");
    SDL_GPUTextureTransferInfo texture_transfer_info{
      .transfer_buffer = texture_transfer_buffer, .offset = 0, .pixels_per_row = 0, .rows_per_layer = 0};
    SDL_GPUTextureRegion texture_region{.texture = texture,
                                        .mip_level = 0,
                                        .layer = 0,
                                        .x = 0,
                                        .y = 0,
                                        .z = 0,
                                        .w = image.width,
                                        .h = image.height,
                                        .d = 1};
    SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    SDL_ReleaseGPUTransferBuffer(device, texture_transfer_buffer);
    return graphics_cache.texture.emplace(key, graphics_cache::cached<SDL_GPUTexture *>{texture, graphics_cache.stamp})
      .first->second.value;
  }

  std::int64_t active::seconds_to_frames(const double seconds) const
  {
    if (seconds <= 0.0 || frequency <= 0) return 0;
    return static_cast<std::int64_t>(seconds * static_cast<double>(frequency));
  }

  double active::frames_to_seconds(const std::int64_t frames) const
  {
    if (frames <= 0 || frequency <= 0) return 0.0;
    return static_cast<double>(frames) / static_cast<double>(frequency);
  }

  double active::gain(const double volume)
  {
    if (volume <= 0.0) return 0.0;
    return volume * volume;
  }

  MIX_Audio *active::require_audio(const unsigned char *data, const std::size_t size, const bool predecode)
  {
    const audio_track::audio_key key{data, size};
    if (const auto iterator{audio_cache.find(key)}; iterator != audio_cache.end()) return iterator->second;
    csp::verify(data, size);
    auto *source{SDL_IOFromConstMem(data, size)};
    if (!source) throw sdl_exception("Could not open audio data for game");
    auto *audio{MIX_LoadAudio_IO(audio_handle, source, predecode, true)};
    if (!audio) throw sdl_exception("Could not load audio for game");
    return audio_cache.emplace(key, audio).first->second;
  }
}

namespace cse
{
  game::game(const initial &initial_)
    : previous{initial_.tick,  initial_.frame,  initial_.aspect, initial_.resolution,
               initial_.clear, initial_.master, initial_.sound,  initial_.music},
      active{initial_.tick,  initial_.frame,  initial_.aspect, initial_.resolution,
             initial_.clear, initial_.master, initial_.sound,  initial_.music}
  {
  }

  scene &game::current(const name scene_name)
  {
    auto scene{find(active.scenes, scene_name)};
    if (active.phase == help::phase::CREATED)
      next.scene = {scene_name, {}};
    else
    {
      active.scene = scene;
      previous.scene = scene;
    }
    return *scene;
  }

  void game::run()
  {
    prepare();
    create();
    while (running())
    {
      step();
      while (behind())
      {
        tps();
        synchronize();
        event();
        simulate();
        collide();
        tps();
      }
      if (ready())
      {
        fps();
        render();
        mix();
        fps();
      }
    }
    destroy();
    clean();
  }

  void game::pre_prepare() {}
  void game::post_prepare() {}
  void game::prepare()
  {
    if (active.phase != help::phase::CLEANED) throw exception("Game must be cleaned before preparation");
    if (!active.window) throw exception("No window has been set for the game");
    if (active.scenes.empty()) throw exception("No scenes have been added to the game");
    if (!active.scene) throw exception("No current scene has been set for the game");
    pre_prepare();
    active.prepare();
    active.window->prepare();
    for (const auto &scene : active.scenes) scene->prepare();
    for (const auto &interface : active.interfaces) interface->prepare();
    active.phase = help::phase::PREPARED;
    post_prepare();
  }

  void game::pre_create() {}
  void game::post_create() {}
  void game::create()
  {
    if (active.phase != help::phase::PREPARED) throw exception("Game must be prepared before creation");
    pre_create();
    active.create();
    active.window->create(active.device, active.aspect.value, active.resolution);
    active.scene->create();
    for (const auto &interface : active.interfaces) interface->create();
    active.phase = help::phase::CREATED;
    post_create();
  }

  void game::pre_synchronize() {}
  void game::post_synchronize() {}
  void game::synchronize()
  {
    if (active.phase != help::phase::CREATED) throw exception("Game must be created before synchronization");
    pre_synchronize();
    active.synchronize(previous);
    if (next.window.has_value())
    {
      if (auto &window{next.window.value()})
      {
        for (const auto &interface : active.interfaces) interface->destroy();
        active.scene->destroy();
        active.window->destroy(active.device);
        active.destroy();
        active.window->clean();
        active.window = window;
        window->prepare();
        active.create();
        window->create(active.device, active.aspect.value, active.resolution);
        active.scene->create();
        for (const auto &interface : active.interfaces) interface->create();
      }
      else
        throw exception("Tried to set window to null");
      next.window.reset();
    }
    active.window->synchronize();
    if (next.scene.has_value())
    {
      if (auto &[name, scene]{next.scene.value()}; scene)
      {
        active.scene->destroy();
        if (name == active.scene->name) active.scene->clean();
        set_or_add(active.scenes, scene);
        active.scene = scene;
        scene->prepare();
        scene->create();
      }
      else
      {
        auto next_scene{find(active.scenes, name)};
        if (name != active.scene->name)
        {
          active.scene->destroy();
          active.scene = next_scene;
          next_scene->create();
        }
      }
      next.scene.reset();
    }
    active.scene->synchronize();
    if (!active.interface_removals.empty())
    {
      for (const auto &interface_name : active.interface_removals)
        if (auto iterator{try_iterate(active.interfaces, interface_name)}; iterator != active.interfaces.end())
        {
          const auto &interface{*iterator};
          if (interface->active.phase == help::phase::CREATED) interface->destroy();
          interface->clean();
          active.interfaces.erase(iterator);
        }
      active.interface_removals.clear();
    }
    if (!active.interface_additions.empty())
    {
      for (auto &interface : active.interface_additions)
      {
        set_or_add(active.interfaces, interface);
        interface->prepare();
        interface->create();
      }
      active.interface_additions.clear();
    }
    for (const auto &interface : active.interfaces) interface->synchronize();
    active.generate_simulation_order();
    active.generate_pool();
    post_synchronize();
  }

  void game::pre_event(const SDL_Event &) {}
  void game::post_event(const SDL_Event &) {}
  void game::event()
  {
    if (active.phase != help::phase::CREATED) throw exception("Game must be created before processing events");
    active.window->active.poll(active.aspect.value, active.resolution);
    while (SDL_PollEvent(&active.window->active.event))
    {
      pre_event(active.window->active.event);
      active.interact();
      active.window->event(active.device);
      active.scene->event(active.window->active.event);
      for (const auto &interface : active.interface_order) interface->event(active.window->active.event);
      post_event(active.window->active.event);
    }
    active.hover();
  }

  void game::pre_simulate(const double) {}
  void game::post_simulate(const double) {}
  void game::simulate()
  {
    if (active.phase != help::phase::CREATED) throw exception("Game must be created before simulation");
    pre_simulate(active.actual_tick);
    active.timer.update(active.actual_tick);
    active.window->simulate(active.actual_tick);
    active.scene->simulate(active.actual_tick);
    for (const auto &interface : active.interface_order) interface->simulate(active.actual_tick);
    post_simulate(active.actual_tick);
  }

  void game::pre_collide(const double) {}
  void game::post_collide(const double) {}
  void game::collide()
  {
    if (active.phase != help::phase::CREATED) throw exception("Game must be created before collision");
    pre_collide(active.actual_tick);
    active.scene->collide(active.actual_tick);
    post_collide(active.actual_tick);
  }

  void game::pre_render(const double) {}
  void game::post_render(const double) {}
  void game::render()
  {
    if (active.phase != help::phase::CREATED) throw exception("Game must be created before rendering");
    pre_render(active.alpha);
    const auto clear{active.clear.interpolated(previous.clear, active.alpha)};
    const auto aspect{active.aspect.interpolated(previous.aspect, active.alpha)};
    if (!active.window->start_render(active.device, aspect, clear, active.alpha)) return;
    active.scene->render(aspect, active.alpha);
    active.render(previous.aspect);
    active.window->end_render(active.alpha);
    post_render(active.alpha);
  }

  void game::pre_mix(const double) {}
  void game::post_mix(const double) {}
  void game::mix()
  {
    if (active.phase != help::phase::CREATED) throw exception("Game must be created before mixing");
    pre_mix(active.alpha);
    active.mix(previous.mixer, previous.master, previous.sound, previous.music);
    post_mix(active.alpha);
  }

  void game::pre_destroy() {}
  void game::post_destroy() {}
  void game::destroy()
  {
    if (active.phase != help::phase::CREATED) throw exception("Game must be created before destruction");
    pre_destroy();
    for (const auto &interface : active.interfaces) interface->destroy();
    active.scene->destroy();
    active.window->destroy(active.device);
    active.destroy();
    active.phase = help::phase::PREPARED;
    post_destroy();
  }

  void game::pre_clean() {}
  void game::post_clean() {}
  void game::clean()
  {
    if (active.phase != help::phase::PREPARED) throw exception("Game must be prepared before cleaning");
    pre_clean();
    for (const auto &interface : active.interfaces) interface->clean();
    for (const auto &scene : active.scenes) scene->clean();
    active.window->clean();
    active.clean();
    active.phase = help::phase::CLEANED;
    post_clean();
  }

  void game::time() { active.time = static_cast<double>(SDL_GetTicksNS()) / 1e9; }

  void game::step()
  {
    active.tick.target = std::max(10.0, active.tick.target);
    active.frame.target = std::max(1.0, active.frame.target);
    const double real_tick = 1.0 / active.tick.target;
    const double real_frame = 1.0 / active.frame.target;
    if (!equal(real_tick, active.actual_tick))
    {
      active.accumulator = active.accumulator * (real_tick / active.actual_tick);
      active.actual_tick = real_tick;
    }
    if (!equal(real_frame, active.actual_frame)) active.actual_frame = real_frame;

    time();
    static double simulation_time{};
    double delta_time{active.time - simulation_time};
    simulation_time = active.time;
    if (delta_time > 0.1) delta_time = 0.1;
    active.accumulator += delta_time;
  }

  bool game::running() { return active.window->active.running; }

  bool game::behind()
  {
    if (active.accumulator >= active.actual_tick)
    {
      active.accumulator -= active.actual_tick;
      return true;
    }
    return false;
  }

  bool game::ready()
  {
    time();
    static double deadline{};
    if (active.time - deadline >= active.actual_frame)
    {
      deadline += active.actual_frame;
      if (active.time - deadline >= active.actual_frame) deadline = active.time;
      active.alpha = active.accumulator / active.actual_tick;
      return true;
    }
    return false;
  }

  void game::tps()
  {
    static std::optional<double> start{};
    static double deadline{};
    static double accumulator{};
    static unsigned int count{};
    if (!start)
    {
      start = static_cast<double>(SDL_GetTicksNS()) / 1e9;
      return;
    }

    count++;
    accumulator += (static_cast<double>(SDL_GetTicksNS()) / 1e9) - start.value();
    if (active.time - deadline >= 1.0)
    {
      active.tick.count = count;
      active.tick.average = (accumulator / count) * 1000.0;
      deadline = active.time;
      accumulator = 0.0;
      count = 0;
    }
    start.reset();
  }

  void game::fps()
  {
    static std::optional<double> start{};
    static double deadline{};
    static double accumulator{};
    static unsigned int count{};
    if (!start)
    {
      start = static_cast<double>(SDL_GetTicksNS()) / 1e9;
      return;
    }

    count++;
    accumulator += (static_cast<double>(SDL_GetTicksNS()) / 1e9) - start.value();
    if (active.time - deadline >= 1.0)
    {
      active.frame.count = count;
      active.frame.average = (accumulator / count) * 1000.0;
      deadline = active.time;
      accumulator = 0.0;
      count = 0;
    }
    start.reset();
  }
}
