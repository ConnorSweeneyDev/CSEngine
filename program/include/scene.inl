#pragma once

#include "scene.hpp"

#include <memory>
#include <type_traits>

#include "camera.hpp"
#include "container.hpp"
#include "core.hpp"
#include "exception.hpp"
#include "interface.hpp"
#include "light.hpp"
#include "name.hpp"
#include "object.hpp"

namespace cse
{
  template <trait::is_camera camera_type, typename... camera_arguments>
  camera_type &scene::set(camera_arguments &&...arguments)
  {
    auto camera{std::make_shared<camera_type>(std::forward<camera_arguments>(arguments)...)};
    if (!game) throw exception("Scene camera added before scene was attached to a game");
    camera->game = game;
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
    return *camera;
  }

  template <trait::is_interface interface_type, typename... interface_arguments>
  interface_type &scene::set(const cse::name interface_name, interface_arguments &&...arguments)
  {
    auto interface{std::make_shared<interface_type>(std::forward<interface_arguments>(arguments)...)};
    interface->name = interface_name;
    if (!game)
      throw exception("Scene interface '{}' added before scene was attached to a game", interface_name.string());
    interface->game = game;
    interface->scene = this;
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

  template <trait::is_object object_type, typename... object_arguments>
  object_type &scene::set(const cse::name object_name, object_arguments &&...arguments)
  {
    auto object{std::make_shared<object_type>(std::forward<object_arguments>(arguments)...)};
    object->name = object_name;
    if (!game) throw exception("Scene object '{}' added before scene was attached to a game", object_name.string());
    object->game = game;
    object->scene = this;
    switch (active.phase)
    {
      case help::phase::CLEANED: active.objects.set(object); break;
      case help::phase::PREPARED:
        if (auto existing{active.objects.find(object_name)}) existing->clean();
        active.objects.set(object);
        object->prepare();
        break;
      case help::phase::CREATED:
        if (active.objects.contains(object_name)) active.object_removals.insert(object_name);
        active.object_additions.set(object);
        break;
    }
    return *object;
  }

  template <trait::is_light light_type, typename... light_arguments>
  light_type &scene::set(const cse::name light_name, light_arguments &&...arguments)
  {
    auto light{std::make_shared<light_type>(std::forward<light_arguments>(arguments)...)};
    light->name = light_name;
    if (!game) throw exception("Scene light '{}' added before scene was attached to a game", light_name.string());
    light->game = game;
    light->scene = this;
    switch (active.phase)
    {
      case help::phase::CLEANED: active.lights.set(light); break;
      case help::phase::PREPARED:
        if (auto existing{active.lights.find(light_name)}) existing->clean();
        active.lights.set(light);
        light->prepare();
        break;
      case help::phase::CREATED:
        if (active.lights.contains(light_name)) active.light_removals.insert(light_name);
        active.light_additions.set(light);
        break;
    }
    return *light;
  }

  template <typename... target_types>
    requires((sizeof...(target_types) == 0) || ((std::is_void_v<target_types> || trait::is_object<target_types> ||
                                                 trait::is_light<target_types> || trait::is_interface<target_types>) &&
                                                ...))
  void scene::remove(const cse::name target_name)
  {
    constexpr bool all{sizeof...(target_types) == 0 || (std::is_void_v<target_types> || ...)};
    constexpr bool interfaces{all || (trait::is_interface<target_types> || ...)};
    constexpr bool objects{all || (trait::is_object<target_types> || ...)};
    constexpr bool lights{all || (trait::is_light<target_types> || ...)};
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
    if constexpr (objects)
      if (auto object{active.objects.find(target_name)})
      {
        if (active.phase == help::phase::CREATED)
          active.object_removals.insert(target_name);
        else
        {
          if (object->active.phase == help::phase::PREPARED) object->clean();
          active.objects.remove(target_name);
        }
      }
    if constexpr (lights)
      if (auto light{active.lights.find(target_name)})
      {
        if (active.phase == help::phase::CREATED)
          active.light_removals.insert(target_name);
        else
        {
          if (light->active.phase == help::phase::PREPARED) light->clean();
          active.lights.remove(target_name);
        }
      }
  }
}
