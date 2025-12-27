#include "scene.hpp"

#include <memory>
#include <utility>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "camera.hpp"
#include "id.hpp"
#include "object.hpp"

namespace cse
{
  scene::~scene()
  {
    hooks.clear();
    objects.clear();
    camera.reset();
    parent.reset();
  }

  bool scene::remove_object(const help::id name)
  {
    if (objects.contains(name))
    {
      pending_removals.insert(name);
      return true;
    }
    return false;
  }

  void scene::initialize(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    hooks.call<void()>("pre_initialize");
    if (!camera->initialized) camera->initialize();
    for (const auto &[name, object] : objects)
      if (!object->initialized) object->initialize(instance, gpu);
    initialized = true;
    hooks.call<void()>("post_initialize");
  }

  void scene::event(const SDL_Event &event)
  {
    hooks.call<void(const SDL_Event &)>("pre_event", event);
    camera->event(event);
    for (const auto &object : objects) object.second->event(event);
    process_removals();
    hooks.call<void(const SDL_Event &)>("post_event", event);
    process_removals();
  }

  void scene::input(const bool *keys)
  {
    hooks.call<void(const bool *)>("pre_input", keys);
    camera->input(keys);
    for (const auto &object : objects) object.second->input(keys);
    process_removals();
    hooks.call<void(const bool *)>("post_input", keys);
    process_removals();
  }

  void scene::simulate(const double simulation_alpha)
  {
    hooks.call<void(const double)>("pre_simulate", simulation_alpha);
    camera->simulate(simulation_alpha);
    for (const auto &object : objects) object.second->simulate(simulation_alpha);
    process_removals();
    hooks.call<void(const double)>("post_simulate", simulation_alpha);
    process_removals();
  }

  void scene::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                     const float target_aspect_ratio, const float global_scale_factor)
  {
    hooks.call<void()>("pre_render");
    auto matrices = camera->render(target_aspect_ratio, global_scale_factor);
    for (const auto &object : objects)
      object.second->render(gpu, command_buffer, render_pass, matrices.first, matrices.second, global_scale_factor);
    hooks.call<void()>("post_render");
  }

  void scene::cleanup(SDL_GPUDevice *gpu)
  {
    hooks.call<void()>("pre_cleanup");
    if (camera->initialized) camera->cleanup();
    for (const auto &[name, object] : objects)
      if (object->initialized) object->cleanup(gpu);
    initialized = false;
    hooks.call<void()>("post_cleanup");
  }

  void scene::process_removals()
  {
    for (const auto &name : pending_removals)
      if (auto iterator{objects.find(name)}; iterator != objects.end())
      {
        const auto &object{iterator->second};
        if (initialized && object->initialized)
          if (auto game{parent.lock()}) object->cleanup(game->window->graphics.gpu);
        objects.erase(iterator);
      }
    pending_removals.clear();
  }
}
