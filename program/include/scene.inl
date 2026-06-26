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

namespace cse
{
  template <trait::is_camera camera_type, typename... camera_arguments>
  scene &scene::set(camera_arguments &&...arguments)
  {
    auto camera{std::make_shared<camera_type>(std::forward<camera_arguments>(arguments)...)};
    camera->scene = this;
    switch (active.phase)
    {
      case help::phase::CLEANED:
        active.camera = camera;
        previous.camera = camera;
        break;
      case help::phase::PREPARED:
        active.camera->clean();
        active.camera = camera;
        camera->prepare();
        break;
      case help::phase::CREATED: next.camera = camera; break;
    }
    return *this;
  }

  template <trait::is_object object_type, typename... object_arguments>
  scene &scene::set(const cse::name object_name, object_arguments &&...arguments)
  {
    auto object{std::make_shared<object_type>(std::forward<object_arguments>(arguments)...)};
    object->name = object_name;
    object->scene = this;
    switch (active.phase)
    {
      case help::phase::CLEANED: set_or_add(active.objects, object); break;
      case help::phase::PREPARED:
        if (auto existing{try_find(active.objects, object_name)}) existing->clean();
        set_or_add(active.objects, object);
        object->prepare();
        break;
      case help::phase::CREATED:
        if (try_contains(active.objects, object_name)) active.object_removals.insert(object_name);
        set_or_add(active.object_additions, object);
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
    return *this;
  }

  template <typename target_type>
    requires(std::is_void_v<target_type> || trait::is_object<target_type> || trait::is_interface<target_type>)
  scene &scene::remove(const cse::name target_name)
  {
    if constexpr (std::is_void_v<target_type> || trait::is_object<target_type>)
      if (auto iterator{try_iterate(active.objects, target_name)}; iterator != active.objects.end())
      {
        if (auto &object{*iterator}; active.phase == help::phase::CREATED)
          active.object_removals.insert(target_name);
        else
        {
          if (object->active.phase == help::phase::PREPARED) object->clean();
          active.objects.erase(iterator);
        }
        return *this;
      }
    if constexpr (std::is_void_v<target_type> || trait::is_interface<target_type>)
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
    return *this;
  }
}
