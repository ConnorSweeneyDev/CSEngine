#include "scene.hpp"

#include <memory>
#include <utility>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "camera.hpp"
#include "exception.hpp"
#include "name.hpp"
#include "object.hpp"
#include "state.hpp"

namespace cse
{
  scene::~scene() { hook.reset(); }

  std::shared_ptr<scene> scene::remove_object(const help::name name)
  {
    if (auto iterator{state.active.objects.find(name)}; iterator != state.active.objects.end())
    {
      auto &object{iterator->second};
      if (state.active.phase == help::phase::CREATED)
        state.removals.insert(name);
      else
      {
        if (object->state.active.phase == help::phase::PREPARED) object->clean();
        state.active.objects.erase(iterator);
      }
    }
    return shared_from_this();
  }

  void scene::prepare()
  {
    if (state.active.phase != help::phase::CLEANED) throw exception("Scene must be cleaned before preparation");
    hook.call<void()>(hooks::PRE_PREPARE);
    if (!state.active.camera) throw exception("Scene must have a camera to be prepared");
    state.active.camera->prepare();
    for (const auto &[name, object] : state.active.objects) object->prepare();
    state.active.phase = help::phase::PREPARED;
    hook.call<void()>(hooks::POST_PREPARE);
  }

  void scene::create(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Scene must be prepared before creation");
    hook.call<void()>(hooks::PRE_CREATE);
    state.active.camera->create();
    for (const auto &[name, object] : state.active.objects) object->create(instance, gpu);
    state.active.phase = help::phase::CREATED;
    hook.call<void()>(hooks::POST_CREATE);
  }

  void scene::previous()
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Scene must be created before updating previous state");
    state.update_previous();
    state.active.camera->previous();
    for (const auto &[name, object] : state.active.objects) object->previous();
  }

  void scene::sync(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Scene must be created before syncing");
    hook.call<void()>(hooks::PRE_SYNC);
    if (state.next.camera.has_value())
    {
      auto &new_camera{state.next.camera.value()};
      state.active.camera->destroy();
      state.active.camera->clean();
      state.active.camera = new_camera;
      new_camera->prepare();
      new_camera->create();
      state.next.camera.reset();
    }
    if (!state.removals.empty())
    {
      for (const auto &name : state.removals)
        if (auto iterator{state.active.objects.find(name)}; iterator != state.active.objects.end())
        {
          const auto &object{iterator->second};
          if (object->state.active.phase == help::phase::CREATED) object->destroy(gpu);
          object->clean();
          state.active.objects.erase(iterator);
        }
      state.removals.clear();
    }
    if (!state.additions.empty())
    {
      for (auto &[name, object] : state.additions)
      {
        state.active.objects.insert_or_assign(name, object);
        object->prepare();
        object->create(instance, gpu);
      }
      state.additions.clear();
    }
    hook.call<void()>(hooks::POST_SYNC);
  }

  void scene::event(const SDL_Event &event)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Scene must be created before processing events");
    hook.call<void(const SDL_Event &)>(hooks::PRE_EVENT, event);
    state.active.camera->event(event);
    for (const auto &[name, object] : state.active.objects) object->event(event);
    hook.call<void(const SDL_Event &)>(hooks::POST_EVENT, event);
  }

  void scene::input(const bool *input)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Scene must be created before processing input");
    hook.call<void(const bool *)>(hooks::PRE_INPUT, input);
    state.active.camera->input(input);
    for (const auto &[name, object] : state.active.objects) object->input(input);
    hook.call<void(const bool *)>(hooks::POST_INPUT, input);
  }

  void scene::simulate(const float poll_rate)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Scene must be created before simulation");
    hook.call<void(const float)>(hooks::PRE_SIMULATE, poll_rate);
    state.active.camera->simulate(poll_rate);
    for (const auto &[name, object] : state.active.objects) object->simulate(poll_rate);
    hook.call<void(const float)>(hooks::POST_SIMULATE, poll_rate);
  }

  void scene::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                     const double alpha, const float aspect_ratio)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Scene must be created before rendering");
    hook.call<void(const double)>(hooks::PRE_RENDER, alpha);
    auto matrices = state.active.camera->render(alpha, aspect_ratio);
    for (const auto &object : graphics.generate_render_order(state.active.camera, state.active.objects, alpha))
      object->render(gpu, command_buffer, render_pass, matrices.first, matrices.second, alpha);
    hook.call<void(const double)>(hooks::POST_RENDER, alpha);
  }

  void scene::destroy(SDL_GPUDevice *gpu)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Scene must be created before destruction");
    hook.call<void()>(hooks::PRE_DESTROY);
    for (const auto &[name, object] : state.active.objects) object->destroy(gpu);
    state.active.camera->destroy();
    state.active.phase = help::phase::PREPARED;
    hook.call<void()>(hooks::POST_DESTROY);
  }

  void scene::clean()
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Scene must be prepared before cleaning");
    hook.call<void()>(hooks::PRE_CLEAN);
    for (const auto &[name, object] : state.active.objects) object->clean();
    state.active.camera->clean();
    state.active.phase = help::phase::CLEANED;
    hook.call<void()>(hooks::POST_CLEAN);
  }
}
