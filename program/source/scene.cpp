#include "scene.hpp"

#include <memory>
#include <utility>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "camera.hpp"
#include "exception.hpp"
#include "game.hpp"
#include "name.hpp"
#include "object.hpp"

namespace cse
{
  scene::~scene() { hook.reset(); }

  std::shared_ptr<scene> scene::remove_object(const help::name name)
  {
    if (!state.initialized)
    {
      if (auto iterator{state.active.objects.find(name)}; iterator != state.active.objects.end())
        state.active.objects.erase(iterator);
      return shared_from_this();
    }
    if (state.active.objects.contains(name)) state.removals.insert(name);
    return shared_from_this();
  }

  void scene::initialize(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    hook.call<void()>("pre_initialize");
    if (!state.active.camera) throw exception("Scene must have a camera to be initialized");
    if (!state.active.camera->state.initialized) state.active.camera->initialize();
    for (const auto &[name, object] : state.active.objects)
      if (!state.removals.contains(name) && !object->state.initialized) object->initialize(instance, gpu);
    state.initialized = true;
    hook.call<void()>("post_initialize");
  }

  void scene::previous()
  {
    state.update_previous();
    state.active.camera->previous();
    for (const auto &[name, object] : state.active.objects)
      if (!state.removals.contains(name)) object->previous();
  }

  void scene::update()
  {
    hook.call<void()>("pre_update");
    if (state.next.camera.has_value())
    {
      if (state.active.camera->state.initialized) state.active.camera->cleanup();
      state.active.camera = state.next.camera.value();
      if (!state.active.camera->state.initialized) state.active.camera->initialize();
      state.next.camera.reset();
    }
    auto game{state.active.parent.lock()};
    if (!state.removals.empty())
    {
      for (const auto &name : state.removals)
        if (auto iterator{state.active.objects.find(name)}; iterator != state.active.objects.end())
        {
          const auto &object{iterator->second};
          if (object->state.initialized)
            if (game) object->cleanup(game->state.active.window->graphics.gpu);
          state.active.objects.erase(iterator);
        }
      state.removals.clear();
    }
    if (!state.additions.empty())
    {
      for (auto &[name, object] : state.additions)
      {
        state.active.objects.insert_or_assign(name, object);
        if (!object->state.initialized)
          if (game)
            object->initialize(game->state.active.window->graphics.instance, game->state.active.window->graphics.gpu);
      }
      state.additions.clear();
    }
    hook.call<void()>("post_update");
  }

  void scene::event(const SDL_Event &event)
  {
    hook.call<void(const SDL_Event &)>("pre_event", event);
    if (!state.active.camera->state.initialized) throw exception("Camera is not initialized");
    state.active.camera->event(event);
    for (const auto &[name, object] : state.active.objects)
      if (!state.removals.contains(name))
      {
        if (!object->state.initialized) throw exception("Object is not initialized");
        object->event(event);
      }
    hook.call<void(const SDL_Event &)>("post_event", event);
  }

  void scene::input(const bool *input)
  {
    hook.call<void(const bool *)>("pre_input", input);
    if (!state.active.camera->state.initialized) throw exception("Camera is not initialized");
    state.active.camera->input(input);
    for (const auto &[name, object] : state.active.objects)
      if (!state.removals.contains(name))
      {
        if (!object->state.initialized) throw exception("Object is not initialized");
        object->input(input);
      }
    hook.call<void(const bool *)>("post_input", input);
  }

  void scene::simulate(const float poll_rate)
  {
    hook.call<void(const float)>("pre_simulate", poll_rate);
    if (!state.active.camera->state.initialized) throw exception("Camera is not initialized");
    state.active.camera->simulate(poll_rate);
    for (const auto &[name, object] : state.active.objects)
      if (!state.removals.contains(name))
      {
        if (!object->state.initialized) throw exception("Object is not initialized");
        object->simulate(poll_rate);
      }
    hook.call<void(const float)>("post_simulate", poll_rate);
  }

  void scene::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                     const double alpha, const float aspect_ratio)
  {
    hook.call<void(const double)>("pre_render", alpha);
    if (!state.active.camera->state.initialized) throw exception("Camera is not initialized");
    auto matrices = state.active.camera->render(alpha, aspect_ratio);
    for (const auto &object :
         graphics.generate_render_order(state.active.camera, state.active.objects, state.removals, alpha))
    {
      if (!object->state.initialized) throw exception("Object is not initialized");
      object->render(gpu, command_buffer, render_pass, matrices.first, matrices.second, alpha);
    }
    hook.call<void(const double)>("post_render", alpha);
  }

  void scene::cleanup(SDL_GPUDevice *gpu)
  {
    hook.call<void()>("pre_cleanup");
    for (const auto &[name, object] : state.active.objects)
    {
      if (!object->state.initialized) throw exception("Object is not initialized");
      object->cleanup(gpu);
    }
    if (!state.active.camera->state.initialized) throw exception("Camera is not initialized");
    state.active.camera->cleanup();
    state.initialized = false;
    hook.call<void()>("post_cleanup");
  }
}
