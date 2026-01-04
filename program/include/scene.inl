#pragma once

#include "scene.hpp"

#include <memory>

#include "camera.hpp"
#include "name.hpp"
#include "traits.hpp"

namespace cse
{
  template <help::is_camera camera_type, typename... camera_arguments>
  std::shared_ptr<scene> scene::set_camera(camera_arguments &&...arguments)
  {
    if (!state.initialized)
    {
      state.active.camera = std::make_shared<camera_type>(std::forward<camera_arguments>(arguments)...);
      state.active.camera->state.active.parent = weak_from_this();
      return shared_from_this();
    }
    state.next.camera = std::make_shared<camera_type>(std::forward<camera_arguments>(arguments)...);
    state.next.camera.value()->state.active.parent = weak_from_this();
    return shared_from_this();
  }

  template <help::is_object object_type, typename... object_arguments>
  std::shared_ptr<scene> scene::set_object(const help::name name, object_arguments &&...arguments)
  {
    if (state.active.objects.contains(name)) state.removals.insert(name);
    auto object{std::make_shared<object_type>(std::forward<object_arguments>(arguments)...)};
    object->state.active.parent = weak_from_this();
    if (!state.initialized)
    {
      state.active.objects.insert_or_assign(name, object);
      return shared_from_this();
    }
    state.additions.insert_or_assign(name, object);
    return shared_from_this();
  }
}
