#include "scene.hpp"

#include <memory>
#include <utility>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "camera.hpp"
#include "exception.hpp"
#include "game.hpp"
#include "id.hpp"
#include "object.hpp"

namespace cse
{
  scene::~scene()
  {
    additions.clear();
    removals.clear();
    hook.reset();
    objects.clear();
    camera.reset();
    parent.reset();
  }

  void scene::remove_object(const help::id name)
  {
    if (!initialized)
    {
      if (auto iterator{objects.find(name)}; iterator != objects.end()) objects.erase(iterator);
      return;
    }
    if (objects.contains(name)) removals.insert(name);
  }

  void scene::initialize(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    hook.call<void()>("pre_initialize");
    if (!camera) throw exception("Scene must have a camera to be initialized");
    if (!camera->initialized) camera->initialize();
    for (const auto &[name, object] : objects)
      if (!removals.contains(name) && !object->initialized) object->initialize(instance, gpu);
    initialized = true;
    hook.call<void()>("post_initialize");
  }

  void scene::event(const SDL_Event &event)
  {
    hook.call<void(const SDL_Event &)>("pre_event", event);
    if (!camera->initialized) throw exception("Camera is not initialized");
    camera->event(event);
    for (const auto &[name, object] : objects)
      if (!removals.contains(name))
      {
        if (!object->initialized) throw exception("Object is not initialized");
        object->event(event);
      }
    hook.call<void(const SDL_Event &)>("post_event", event);
  }

  void scene::input(const bool *keys)
  {
    hook.call<void(const bool *)>("pre_input", keys);
    camera->input(keys);
    for (const auto &[name, object] : objects)
      if (!removals.contains(name)) object->input(keys);
    hook.call<void(const bool *)>("post_input", keys);
  }

  void scene::simulate(const double poll_rate)
  {
    hook.call<void()>("pre_simulate");
    camera->simulate();
    for (const auto &[name, object] : objects)
      if (!removals.contains(name)) object->simulate(poll_rate);
    hook.call<void()>("post_simulate");
  }

  void scene::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                     const double alpha, const float aspect_ratio, const float scale_factor)
  {
    hook.call<void()>("pre_render");
    auto matrices = camera->render(alpha, aspect_ratio, scale_factor);
    for (const auto &[name, object] : objects)
      if (!removals.contains(name))
        object->render(gpu, command_buffer, render_pass, matrices.first, matrices.second, alpha, scale_factor);
    hook.call<void()>("post_render");
  }

  void scene::cleanup(SDL_GPUDevice *gpu)
  {
    hook.call<void()>("pre_cleanup");
    if (camera->initialized) camera->cleanup();
    for (const auto &[name, object] : objects)
      if (object->initialized) object->cleanup(gpu);
    initialized = false;
    hook.call<void()>("post_cleanup");
  }

  void scene::process_updates()
  {
    if (removals.empty() && additions.empty()) return;
    auto game{parent.lock()};
    for (const auto &name : removals)
      if (auto iterator{objects.find(name)}; iterator != objects.end())
      {
        const auto &object{iterator->second};
        if (initialized && object->initialized)
          if (game) object->cleanup(game->window->graphics.gpu);
        objects.erase(iterator);
      }
    removals.clear();
    if (additions.empty()) return;
    for (auto &[name, object] : additions)
    {
      objects.insert_or_assign(name, object);
      if (initialized && !object->initialized)
        if (game) object->initialize(game->window->graphics.instance, game->window->graphics.gpu);
    }
    additions.clear();
  }

  void scene::update_previous()
  {
    camera->previous.update(camera->state, camera->graphics);
    for (const auto &[name, object] : objects) object->previous.update(object->state, object->graphics);
  }
}
