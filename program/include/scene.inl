#pragma once

#include "scene.hpp"

#include <memory>

#include "camera.hpp"
#include "declaration.hpp"
#include "name.hpp"
#include "object.hpp"
#include "state.hpp"

namespace cse
{
  template <trait::is_camera camera_type, typename... camera_arguments>
  scene &scene::set(camera_arguments &&...arguments)
  {
    auto camera{std::make_shared<camera_type>(std::forward<camera_arguments>(arguments)...)};
    camera->state.active.parent = weak_from_this();
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
  scene &scene::set(const name name, object_arguments &&...arguments)
  {
    auto object{std::make_shared<object_type>(std::forward<object_arguments>(arguments)...)};
    object->state.active.parent = weak_from_this();
    switch (state.active.phase)
    {
      case help::phase::CLEANED: state.active.objects.insert_or_assign(name, object); break;
      case help::phase::PREPARED:
        if (auto iterator{state.active.objects.find(name)}; iterator != state.active.objects.end())
          if (iterator->second->state.active.phase == help::phase::PREPARED) iterator->second->clean();
        state.active.objects.insert_or_assign(name, object);
        object->prepare();
        break;
      case help::phase::CREATED:
        if (state.active.objects.contains(name)) state.removals.insert(name);
        state.additions.insert_or_assign(name, object);
        break;
    }
    return *this;
  }
}
