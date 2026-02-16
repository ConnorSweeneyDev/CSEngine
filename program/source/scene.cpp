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
  scene &scene::remove(const name name)
  {
    if (auto iterator{state.active.objects.find(name)}; iterator != state.active.objects.end())
    {
      if (auto &object{iterator->second}; state.active.phase == help::phase::CREATED)
        state.removals.insert(name);
      else
      {
        if (object->state.active.phase == help::phase::PREPARED) object->clean();
        state.active.objects.erase(iterator);
      }
    }
    return *this;
  }

  void scene::prepare()
  {
    if (state.active.phase != help::phase::CLEANED) throw exception("Scene must be cleaned before preparation");
    pre_prepare();
    if (!state.active.camera) throw exception("Scene must have a camera to be prepared");
    state.active.camera->prepare();
    for (const auto &[name, object] : state.active.objects) object->prepare();
    state.active.phase = help::phase::PREPARED;
    post_prepare();
  }

  void scene::create(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Scene must be prepared before creation");
    pre_create();
    state.active.camera->create();
    for (const auto &[name, object] : state.active.objects) object->create(instance, gpu);
    state.active.phase = help::phase::CREATED;
    post_create();
  }

  void scene::previous()
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Scene must be created before updating previous state");
    pre_previous();
    state.update_previous();
    state.active.camera->previous();
    for (const auto &[name, object] : state.active.objects) object->previous();
    post_previous();
  }

  void scene::sync(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Scene must be created before syncing");
    pre_sync();
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
    post_sync();
  }

  void scene::event(const SDL_Event &event)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Scene must be created before processing events");
    pre_event(event);
    state.active.camera->event(event);
    for (const auto &[name, object] : state.active.objects) object->event(event);
    post_event(event);
  }

  void scene::input(const bool *input)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Scene must be created before processing input");
    pre_input(input);
    state.active.camera->input(input);
    for (const auto &[name, object] : state.active.objects) object->input(input);
    post_input(input);
  }

  void scene::simulate(const double tick)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Scene must be created before simulation");
    pre_simulate(tick);
    state.active.timer.update(tick);
    state.active.camera->simulate(tick);
    for (const auto &[name, object] : state.active.objects) object->simulate(tick);
    for (const auto &[name, object] : state.active.objects) object->collide(tick, name, state.active.objects);
    post_simulate(tick);
  }

  void scene::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                     const double previous_aspect, const double active_aspect, const double alpha)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Scene must be created before rendering");
    pre_render(alpha);
    auto matrices = state.active.camera->render(previous_aspect, active_aspect, alpha);
    for (const auto &object : graphics.generate_render_order(state.active.camera, state.active.objects, alpha))
      object->render(gpu, command_buffer, render_pass, matrices.first, matrices.second, alpha);
    post_render(alpha);
  }

  void scene::destroy(SDL_GPUDevice *gpu)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Scene must be created before destruction");
    pre_destroy();
    for (const auto &[name, object] : state.active.objects) object->destroy(gpu);
    state.active.camera->destroy();
    state.active.phase = help::phase::PREPARED;
    post_destroy();
  }

  void scene::clean()
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Scene must be prepared before cleaning");
    pre_clean();
    for (const auto &[name, object] : state.active.objects) object->clean();
    state.active.camera->clean();
    state.active.phase = help::phase::CLEANED;
    post_clean();
  }
}
