#include "scene.hpp"

#include <memory>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "camera.hpp"
#include "container.hpp"
#include "exception.hpp"
#include "game.hpp"
#include "input.hpp"
#include "interface.hpp"
#include "name.hpp"
#include "object.hpp"
#include "state.hpp"

namespace cse
{
  void scene::pre_prepare() {}
  void scene::post_prepare() {}
  void scene::prepare()
  {
    if (state.active.phase != help::phase::CLEANED)
      throw exception("Scene '{}' must be cleaned before preparation", name.string());
    pre_prepare();
    if (!state.active.camera) throw exception("Scene '{}' must have a camera to be prepared", name.string());
    state.active.camera->prepare();
    for (const auto &object : state.active.objects) object->prepare();
    for (const auto &interface : state.active.interfaces) interface->prepare();
    state.active.phase = help::phase::PREPARED;
    post_prepare();
  }

  void scene::pre_create() {}
  void scene::post_create() {}
  void scene::create()
  {
    if (state.active.phase != help::phase::PREPARED)
      throw exception("Scene '{}' must be prepared before creation", name.string());
    pre_create();
    state.active.camera->create();
    for (const auto &object : state.active.objects) object->create();
    for (const auto &interface : state.active.interfaces) interface->create();
    state.active.phase = help::phase::CREATED;
    post_create();
  }

  void scene::pre_previous() {}
  void scene::post_previous() {}
  void scene::previous()
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Scene '{}' must be created before updating previous state", name.string());
    pre_previous();
    state.update_previous();
    state.active.camera->previous();
    for (const auto &object : state.active.objects) object->previous();
    for (const auto &interface : state.active.interfaces) interface->previous();
    post_previous();
  }

  void scene::pre_sync() {}
  void scene::post_sync() {}
  void scene::sync()
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Scene '{}' must be created before syncing", name.string());
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
    if (!state.object_removals.empty())
    {
      for (const auto &object_name : state.object_removals)
        if (auto iterator{try_iterate(state.active.objects, object_name)}; iterator != state.active.objects.end())
        {
          const auto &object{*iterator};
          if (object->state.active.phase == help::phase::CREATED) object->destroy();
          object->clean();
          state.active.objects.erase(iterator);
        }
      state.object_removals.clear();
    }
    if (!state.object_additions.empty())
    {
      for (auto &object : state.object_additions)
      {
        set_or_add(state.active.objects, object);
        object->prepare();
        object->create();
      }
      state.object_additions.clear();
    }
    if (!state.interface_removals.empty())
    {
      for (const auto &interface_name : state.interface_removals)
        if (auto iterator{try_iterate(state.active.interfaces, interface_name)};
            iterator != state.active.interfaces.end())
        {
          const auto &interface{*iterator};
          if (interface->state.active.phase == help::phase::CREATED) interface->destroy();
          interface->clean();
          state.active.interfaces.erase(iterator);
        }
      state.interface_removals.clear();
    }
    if (!state.interface_additions.empty())
    {
      for (auto &interface : state.interface_additions)
      {
        set_or_add(state.active.interfaces, interface);
        interface->prepare();
        interface->create();
      }
      state.interface_additions.clear();
    }
    state.generate_order(state.active.objects);
    state.generate_order(state.active.interfaces);
    post_sync();
  }

  void scene::pre_event(const SDL_Event &) {}
  void scene::post_event(const SDL_Event &) {}
  void scene::event(const SDL_Event &event)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Scene '{}' must be created before processing events", name.string());
    pre_event(event);
    state.active.camera->event(event);
    for (const auto &object : state.object_order) object->event(event);
    for (const auto &interface : state.interface_order) interface->event(event);
    post_event(event);
  }

  void scene::pre_input(const cse::keyboard &, const cse::mouse &) {}
  void scene::post_input(const cse::keyboard &, const cse::mouse &) {}
  void scene::input(const cse::keyboard &keyboard, const cse::mouse &mouse)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Scene '{}' must be created before processing input", name.string());
    pre_input(keyboard, mouse);
    state.active.camera->input(keyboard, mouse);
    for (const auto &object : state.object_order) object->input(keyboard, mouse);
    for (const auto &interface : state.interface_order) interface->input(keyboard, mouse);
    post_input(keyboard, mouse);
  }

  void scene::pre_simulate(const double) {}
  void scene::post_simulate(const double) {}
  void scene::simulate(const double tick)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Scene '{}' must be created before simulation", name.string());
    pre_simulate(tick);
    state.active.timer.update(tick);
    state.active.camera->simulate(tick);
    for (const auto &object : state.object_order) object->simulate(tick);
    for (const auto &interface : state.interface_order) interface->simulate(tick);
    post_simulate(tick);
  }

  void scene::pre_collide(const double) {}
  void scene::post_collide(const double) {}
  void scene::collide(const double tick)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Scene '{}' must be created before collision", name.string());
    pre_collide(tick);
    for (state.generate_contacts(); const auto &object : state.object_order) object->collide(tick);
    post_collide(tick);
  }

  void scene::pre_render(const double) {}
  void scene::post_render(const double) {}
  void scene::render(SDL_Window *instance, SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer,
                     SDL_GPURenderPass *render_pass, const double previous_aspect, const double active_aspect,
                     const double alpha)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Scene '{}' must be created before rendering", name.string());
    pre_render(alpha);
    graphics.render(instance, gpu, game->graphics, state.active.camera.get(), state.active.objects,
                    state.active.camera->render(previous_aspect, active_aspect, alpha), command_buffer, render_pass,
                    alpha);
    for (const auto &object : graphics.order) object->render(alpha);
    post_render(alpha);
  }

  void scene::pre_destroy() {}
  void scene::post_destroy() {}
  void scene::destroy()
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Scene '{}' must be created before destruction", name.string());
    pre_destroy();
    for (const auto &interface : state.active.interfaces) interface->destroy();
    for (const auto &object : state.active.objects) object->destroy();
    state.active.camera->destroy();
    graphics.order.clear();
    state.active.phase = help::phase::PREPARED;
    post_destroy();
  }

  void scene::pre_clean() {}
  void scene::post_clean() {}
  void scene::clean()
  {
    if (state.active.phase != help::phase::PREPARED)
      throw exception("Scene '{}' must be prepared before cleaning", name.string());
    pre_clean();
    for (const auto &interface : state.active.interfaces) interface->clean();
    for (const auto &object : state.active.objects) object->clean();
    state.active.camera->clean();
    state.active.phase = help::phase::CLEANED;
    post_clean();
  }
}
