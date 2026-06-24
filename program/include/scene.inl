#pragma once

#include "scene.hpp"

#include <memory>
#include <type_traits>

#include "camera.hpp"
#include "container.hpp"
#include "core.hpp"
#include "exception.hpp"
#include "interface.hpp"
#include "name.hpp"
#include "object.hpp"
#include "state.hpp"

namespace cse
{
  template <trait::is_camera camera_type, typename... camera_arguments>
  scene &scene::set(camera_arguments &&...arguments)
  {
    auto camera{std::make_shared<camera_type>(std::forward<camera_arguments>(arguments)...)};
    camera->scene = this;
    switch (state.active.phase)
    {
      case help::phase::CLEANED:
        state.active.camera = camera;
        state.previous.camera = camera;
        break;
      case help::phase::PREPARED:
        state.active.camera->clean();
        state.active.camera = camera;
        camera->prepare();
        break;
      case help::phase::CREATED: state.next.camera = camera; break;
    }
    return *this;
  }

  template <trait::is_object object_type, typename... object_arguments>
  scene &scene::set(const cse::name object_name, object_arguments &&...arguments)
  {
    auto object{std::make_shared<object_type>(std::forward<object_arguments>(arguments)...)};
    object->name = object_name;
    object->scene = this;
    switch (state.active.phase)
    {
      case help::phase::CLEANED: set_or_add(state.active.objects, object); break;
      case help::phase::PREPARED:
        if (auto existing{try_find(state.active.objects, object_name)}) existing->clean();
        set_or_add(state.active.objects, object);
        object->prepare();
        break;
      case help::phase::CREATED:
        if (try_contains(state.active.objects, object_name)) state.object_removals.insert(object_name);
        set_or_add(state.object_additions, object);
        break;
    }
    return *this;
  }

  template <trait::is_interface interface_type, typename... interface_arguments>
  scene &scene::set(const cse::name interface_name, interface_arguments &&...arguments)
  {
    auto interface{std::make_shared<interface_type>(std::forward<interface_arguments>(arguments)...)};
    interface->name = interface_name;
    if (!game)
      throw exception("Scene interface '{}' added before scene was attached to a game", interface_name.string());
    interface->game = game;
    interface->scene = this;
    switch (state.active.phase)
    {
      case help::phase::CLEANED: set_or_add(state.active.interfaces, interface); break;
      case help::phase::PREPARED:
        if (auto existing{try_find(state.active.interfaces, interface_name)}) existing->clean();
        set_or_add(state.active.interfaces, interface);
        interface->prepare();
        break;
      case help::phase::CREATED:
        if (try_contains(state.active.interfaces, interface_name)) state.interface_removals.insert(interface_name);
        set_or_add(state.interface_additions, interface);
        break;
    }
    return *this;
  }

  template <typename target_type>
    requires(std::is_void_v<target_type> || trait::is_object<target_type> || trait::is_interface<target_type>)
  scene &scene::remove(const cse::name target_name)
  {
    if constexpr (std::is_void_v<target_type> || trait::is_object<target_type>)
      if (auto iterator{try_iterate(state.active.objects, target_name)}; iterator != state.active.objects.end())
      {
        if (auto &object{*iterator}; state.active.phase == help::phase::CREATED)
          state.object_removals.insert(target_name);
        else
        {
          if (object->state.active.phase == help::phase::PREPARED) object->clean();
          state.active.objects.erase(iterator);
        }
        return *this;
      }
    if constexpr (std::is_void_v<target_type> || trait::is_interface<target_type>)
      if (auto iterator{try_iterate(state.active.interfaces, target_name)}; iterator != state.active.interfaces.end())
      {
        if (auto &interface{*iterator}; state.active.phase == help::phase::CREATED)
          state.interface_removals.insert(target_name);
        else
        {
          if (interface->state.active.phase == help::phase::PREPARED) interface->clean();
          state.active.interfaces.erase(iterator);
        }
      }
    return *this;
  }
}
