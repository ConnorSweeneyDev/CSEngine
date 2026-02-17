#pragma once

#include "scene.hpp"

#include <memory>

#include "camera.hpp"
#include "container.hpp"
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
    object->state.name = name;
    object->state.active.parent = weak_from_this();
    switch (state.active.phase)
    {
      case help::phase::CLEANED: set_or_add(state.active.objects, object); break;
      case help::phase::PREPARED:
        if (auto existing{try_find(state.active.objects, name)})
          if (existing->state.active.phase == help::phase::PREPARED) existing->clean();
        set_or_add(state.active.objects, object);
        object->prepare();
        break;
      case help::phase::CREATED:
        if (try_contains(state.active.objects, name)) state.removals.insert(name);
        set_or_add(state.additions, object);
        break;
    }
    return *this;
  }
}
