#pragma once

#include "scene.hpp"

#include <memory>

#include "camera.hpp"
#include "id.hpp"
#include "traits.hpp"

namespace cse
{
  template <help::is_camera camera_type, typename... camera_arguments>
  void scene::set_camera(camera_arguments &&...arguments)
  {
    camera = std::make_shared<camera_type>(std::forward<camera_arguments>(arguments)...);
    camera->parent = shared_from_this();
  }

  template <help::is_object object_type, typename... object_arguments>
  void scene::set_object(const help::id name, object_arguments &&...arguments)
  {
    if (objects.contains(name)) removals.insert(name);
    auto object{std::make_shared<object_type>(std::forward<object_arguments>(arguments)...)};
    object->parent = shared_from_this();
    if (!initialized)
    {
      objects.insert_or_assign(name, object);
      return;
    }
    additions.insert_or_assign(name, object);
  }
}
